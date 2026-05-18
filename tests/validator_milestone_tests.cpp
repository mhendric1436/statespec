#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/lexer.hpp"
#include "statespec/parser.hpp"
#include "statespec/source.hpp"
#include "statespec/validator.hpp"

#include <string>
#include <utility>

namespace
{

void require(
    bool condition,
    const char* message
)
{
    INFO(message);
    REQUIRE(condition);
}

statespec::DiagnosticBag validate_text(const std::string& text)
{
    statespec::DiagnosticBag diagnostics;
    statespec::SourceFile source{"validator_test.sspec", text};
    statespec::Lexer lexer{source};
    auto tokens = lexer.lex(diagnostics);
    statespec::Parser parser{std::move(tokens)};
    auto spec = parser.parse(diagnostics);
    statespec::Validator validator;
    validator.validate(spec, diagnostics);
    return diagnostics;
}

bool has_error_code(
    const statespec::DiagnosticBag& diagnostics,
    const std::string& code
)
{
    for (const auto& diagnostic : diagnostics.all())
    {
        if (diagnostic.severity == statespec::DiagnosticSeverity::Error && diagnostic.code == code)
        {
            return true;
        }
    }
    return false;
}

bool has_error_message_containing(
    const statespec::DiagnosticBag& diagnostics,
    const std::string& fragment
)
{
    for (const auto& diagnostic : diagnostics.all())
    {
        if (diagnostic.severity == statespec::DiagnosticSeverity::Error &&
            diagnostic.message.find(fragment) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

bool has_warning_code(
    const statespec::DiagnosticBag& diagnostics,
    const std::string& code
)
{
    for (const auto& diagnostic : diagnostics.all())
    {
        if (diagnostic.severity == statespec::DiagnosticSeverity::Warning &&
            diagnostic.code == code)
        {
            return true;
        }
    }
    return false;
}

void validator_accepts_resolved_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderProcessingRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderProcessingResponse {
            accepted bool
          }
          shape OrderProcessingState {
            attempt int
          }
          log OrderStarted {
            level info
            event_name "order.started"
            fields {
              tenant_id string
              order_id string
            }
          }
          entity Order {
            key tenant_id, order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              order_id string
            }
            state_machine {
              state Creating
              state Active
              state Failed {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Creating
              terminal Failed
              Creating -> Active
              Creating -> Failed
            }
            indexes {
              index by_tenant_status on tenant_id, status
              unique by_tenant_order_id on tenant_id, order_id
            }
          }
          queue EmailQueue {
            namespace workflow_ns
            channel email
            visibility_timeout PT30S
            max_attempts 3
            message SendConfirmation {
              idempotency_key message_id
              payload {
                tenant_id string
                message_id string
                order_id string
              }
            }
          }
          lease WorkerLease {
            resource "worker"
            ttl PT30S
            renew_every PT10S
            holder worker_id
            fencing_token true
            max_ttl PT5M
          }
          workflow OrderProcessing {
            version 1
            singleton false
            expected_execution_time PT60S
            on StartOrderProcessing
            input StartOrderProcessingRequest
            state OrderProcessingState
            load Order by order_id as order
            start validate_order
            step validate_order {
              expected_execution_time PT10S
              max_retries 1
              require order.status == "Creating";
              emit OrderStarted {
                order_id = order.order_id;
              };
              enqueue EmailQueue.SendConfirmation {
                order_id = order.order_id;
              };
              acquire lease WorkerLease as workerLease;
              renew lease WorkerLease;
              release lease WorkerLease;
              start workflow OrderProcessing {
                order_id = order.order_id;
              };
              transition_to validate_order;
              complete;
            }
          }
          worker OrderWorker {
            singleton true
            lease WorkerLease
            polls EmailQueue.SendConfirmation
            executes OrderProcessing
          }
          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderProcessingRequest
            output StartOrderProcessingResponse
            starts workflow OrderProcessing
          }
          policy WorkflowAccess {
            tenant scoped_by tenant_id
            allow StartOrderProcessing when caller.role == operator;
            audit StartOrderProcessing;
          }
        }
    )sspec");

    require(!diagnostics.has_errors(), "validator should accept resolved references");
}

void validator_warns_on_noncanonical_entity_and_workflow_order()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          entity Order {
            key tenant_id, order_id
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              order_id string
            }
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            state_machine {
              state Active
              initial Active
            }
          }

          workflow OrderProcessing {
            version 1
            step process_order {
              expected_execution_time PT10S
              max_retries 1
            }
            singleton false
            expected_execution_time PT30S
            start process_order
          }
        }
    )sspec");

    require(
        !diagnostics.has_errors(), "noncanonical order should warn without invalidating the spec"
    );
    require(has_warning_code(diagnostics, "SSPEC6101"), "validator should warn on entity order");
    require(has_warning_code(diagnostics, "SSPEC6102"), "validator should warn on workflow order");
}

void validator_rejects_invalid_shapes()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          shape createOrderRequest {
            order_id MissingType
            order_id string
          }

          shape EmptyShape {
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3201"), "validator should reject non-PascalCase shapes"
    );
    require(
        has_error_code(diagnostics, "SSPEC3002"),
        "validator should reject unknown shape field types"
    );
    require(
        has_error_code(diagnostics, "SSPEC3001"), "validator should reject duplicate shape fields"
    );
    require(has_error_code(diagnostics, "SSPEC4001"), "validator should reject empty shapes");
}

void validator_accepts_terminal_garbage_collection_modes()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          entity Order {
            key tenant_id, order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              order_id string
            }
            state_machine {
              state Creating
              state Deleted {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: delete
                }
              }
              state Archived {
                terminal: true
                garbage_collection {
                  after: P90D
                  mode: archive
                }
              }
              state Tombstoned {
                terminal: true
                garbage_collection {
                  after: P60D
                  mode: tombstone
                }
              }
              initial Creating
              terminal [Deleted, Archived, Tombstoned]
              Creating -> Deleted
              Creating -> Archived
              Creating -> Tombstoned
            }
          }
        }
    )sspec");

    require(
        !diagnostics.has_errors(),
        "validator should accept supported terminal garbage collection modes"
    );
}

void validator_rejects_duplicate_top_level_names()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key id
            fields { id string }
          }
          entity Order {
            key id
            fields { id string }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3001"),
        "validator should reject duplicate top-level names"
    );
}

void validator_rejects_missing_entity_key_field()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key order_id
            fields { status string }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3002"), "validator should reject unknown entity key field"
    );
}

void validator_rejects_invalid_entity_indexes()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              order_id string
              status string
            }
            indexes {
              index by_status on missing_status
              unique by_status on order_id
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3001"),
        "validator should reject duplicate entity index names"
    );
    require(
        has_error_code(diagnostics, "SSPEC3002"),
        "validator should reject unknown entity index fields"
    );
}

void validator_rejects_missing_entity_management_model()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              order_id string
              updated_at timestamp
              status string
            }
          }
          entity MissingOwnership {
            key missing_id
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              missing_id string
            }
            state_machine {
              state Active
              initial Active
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "field 'created_at'"),
        "validator should require created_at"
    );
    require(
        has_error_message_containing(diagnostics, "state_machine"),
        "validator should require state_machine"
    );
    require(
        has_error_message_containing(diagnostics, "ownership"), "validator should require ownership"
    );
}

void validator_rejects_missing_system_tenancy()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              order_id string
            }
            state_machine {
              state Active
              initial Active
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "tenant scoped_by"),
        "validator should require system tenant scope"
    );
    require(
        has_error_message_containing(diagnostics, "system_tenant configured"),
        "validator should require system tenant configuration"
    );
}

void validator_rejects_missing_tenant_field_propagation()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderRequest {
            order_id string
          }

          entity Order {
            key order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              order_id string
            }
            state_machine {
              state Pending
              initial Pending
            }
          }

          queue OrderQueue {
            namespace workflow_ns
            channel orders
            message StartOrder {
              idempotency_key message_id
              payload {
                message_id string
                order_id string
              }
            }
          }

          api StartOrder {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderRequest
            enqueues OrderQueue.StartOrder
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3401"),
        "validator should require tenant field on tenant-scoped entities"
    );
    require(
        has_error_code(diagnostics, "SSPEC3402"),
        "validator should require tenant field on tenant-scoped queue messages"
    );
    require(
        has_error_code(diagnostics, "SSPEC3403"),
        "validator should require tenant field on tenant-scoped API input shapes"
    );
    require(
        has_error_code(diagnostics, "SSPEC3404"),
        "validator should require tenant field in tenant-scoped entity keys"
    );
}

void validator_rejects_policy_tenant_scope_mismatch()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderResponse {
            accepted bool
          }

          api StartOrder {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderRequest
            output StartOrderResponse
          }

          policy OrderAccess {
            tenant scoped_by account_id
            allow StartOrder when caller.role == operator;
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3405"),
        "validator should reject policy tenant scopes that differ from the system tenant scope"
    );
}

void validator_rejects_incomplete_policy_canonical_shape()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          policy EmptyAccess {
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "tenant scoped_by"),
        "validator should require explicit policy tenant scope"
    );
    require(
        has_error_message_containing(diagnostics, "at least one allow, deny, quota, or audit"),
        "validator should require at least one policy action"
    );
}

void validator_warns_on_noncanonical_policy_order()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderResponse {
            accepted bool
          }

          api StartOrder {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderRequest
            output StartOrderResponse
          }

          policy OrderAccess {
            tenant scoped_by tenant_id
            audit StartOrder;
            allow StartOrder when caller.role == operator;
          }
        }
    )sspec");

    require(!diagnostics.has_errors(), "noncanonical policy order should remain valid");
    require(has_warning_code(diagnostics, "SSPEC6103"), "validator should warn on policy order");
}

void validator_rejects_invalid_entity_management_field_types()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key order_id
            fields {
              order_id string
              created_at string
              updated_at timestamp
              status int
            }
            state_machine {
              state Pending
              initial Pending
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3102"),
        "validator should reject invalid entity management field types"
    );
}

void validator_rejects_noncanonical_entity_management_field_order()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key order_id
            fields {
              order_id string
              created_at timestamp
              updated_at timestamp
              status string
            }
            state_machine {
              state Pending
              initial Pending
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3106"),
        "validator should reject noncanonical entity management field order"
    );
}

void validator_rejects_invalid_terminal_garbage_collection()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key order_id
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              order_id string
            }
            state_machine {
              state Creating {
                garbage_collection {
                  after: soon
                  mode: compact
                }
              }
              state Deleted {
                terminal: true
                garbage_collection {
                  after: P30D
                }
                garbage_collection {
                  after: P60D
                  mode: delete
                }
              }
              state Archived {
                terminal: true
                garbage_collection {
                  mode: archive
                }
              }
              initial Creating
              terminal [Deleted, Archived]
              Creating -> Deleted
              Deleted -> Creating
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3103"),
        "validator should reject garbage collection on non-terminal states"
    );
    require(
        has_error_code(diagnostics, "SSPEC4001"),
        "validator should reject missing garbage collection fields"
    );
    require(
        has_error_code(diagnostics, "SSPEC4004"),
        "validator should reject invalid garbage collection durations"
    );
    require(
        has_error_code(diagnostics, "SSPEC3104"),
        "validator should reject invalid garbage collection modes"
    );
    require(
        has_error_code(diagnostics, "SSPEC3001"),
        "validator should reject duplicate garbage collection policy blocks"
    );
    require(
        has_error_code(diagnostics, "SSPEC3105"),
        "validator should reject outgoing transitions from garbage-collected terminal states"
    );
}

void validator_rejects_implicit_terminal_retention()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              order_id string
            }
            state_machine {
              state Creating
              state InlineOnly {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              state ListedOnly
              state MissingRetention {
                terminal: true
              }
              initial Creating
              terminal [ListedOnly, MissingRetention]
              Creating -> InlineOnly
              Creating -> ListedOnly
              Creating -> MissingRetention
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3107"),
        "validator should require inline terminal states in the terminal list"
    );
    require(
        has_error_code(diagnostics, "SSPEC3108"),
        "validator should require terminal list states to declare terminal: true"
    );
    require(
        has_error_code(diagnostics, "SSPEC3109"),
        "validator should require terminal states to declare garbage_collection"
    );
}

void validator_rejects_unknown_workflow_start_step()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          workflow OrderProcessing {
            version 1
            start missing_step
            step validate_order {
              expected_execution_time PT10S
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3002"),
        "validator should reject unknown workflow start step"
    );
}

void validator_rejects_incomplete_workflow_canonical_shape()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          workflow OrderProcessing {
            version 1
            start validate_order
            step validate_order {
              expected_execution_time PT10S
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "singleton"),
        "validator should require explicit workflow singleton"
    );
    require(
        has_error_message_containing(diagnostics, "expected_execution_time"),
        "validator should require explicit workflow expected_execution_time"
    );
    require(
        has_error_message_containing(diagnostics, "max_retries"),
        "validator should require explicit step max_retries"
    );
}

void validator_rejects_invalid_workflow_behavior_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          shape WorkflowState {
            attempt int
          }

          entity Order {
            key order_id
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              order_id string
            }
            state_machine {
              state Pending
              initial Pending
            }
          }

          workflow OrderProcessing {
            version 1
            on MissingTrigger
            input MissingInput
            state MissingState
            load MissingEntity by order_id as missing
            load Order by missing_key as order
            start validate_order
            step validate_order {
              expected_execution_time PT10S
              require feature_enabled(MissingFlag);
              emit MissingEvent;
              enqueue MissingQueue.Message;
              acquire lease MissingLease;
              start workflow MissingWorkflow;
              transition_to missing_step;
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "unknown workflow trigger reference"),
        "validator should reject unknown workflow trigger"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow input reference"),
        "validator should reject unknown workflow input"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow state reference"),
        "validator should reject unknown workflow state"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow load entity reference"),
        "validator should reject unknown workflow load entity"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow load key field reference"),
        "validator should reject workflow loads by non-key fields"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow emit target reference"),
        "validator should reject unknown workflow emit targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow enqueue target reference"),
        "validator should reject unknown workflow enqueue targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow lease target reference"),
        "validator should reject unknown workflow lease targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow start target reference"),
        "validator should reject unknown workflow start targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow transition target reference"),
        "validator should reject unknown workflow transition targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown feature flag reference"),
        "validator should validate feature flag references in workflow expressions"
    );
}

void validator_rejects_unknown_worker_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          worker WorkerA {
            singleton true
            lease MissingLease
            polls MissingQueue.Message
            executes MissingWorkflow
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3002"),
        "validator should reject unknown worker references"
    );
}

void validator_rejects_unknown_api_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input MissingRequest
            output MissingResponse
            error MissingError
            starts workflow MissingWorkflow
            enqueues MissingQueue.Message
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3002"), "validator should reject unknown API references"
    );
}

void validator_rejects_api_with_multiple_primary_actions()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderResponse {
            accepted bool
          }
          queue OrderQueue {
            namespace orders
            channel order_starts
            visibility_timeout PT30S
            max_attempts 3
            message StartOrder {
              idempotency_key order_id
              payload {
                tenant_id string
                order_id string
              }
            }
          }
          workflow OrderProcessing {
            version 1
            singleton false
            expected_execution_time PT5M
            start process_order
            step process_order {
              expected_execution_time PT10S
              max_retries 1
            }
          }

          api StartOrder {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderRequest
            output StartOrderResponse
            starts workflow OrderProcessing
            enqueues OrderQueue.StartOrder
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4101"),
        "validator should reject APIs with multiple primary actions"
    );
}

void validator_rejects_incomplete_api_canonical_shape()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "input"),
        "validator should require input on mutating APIs"
    );
    require(
        has_error_message_containing(diagnostics, "output"),
        "validator should require output on non-DELETE APIs"
    );
}

void validator_rejects_tenant_scoped_api_path_without_tenant_identity()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderResponse {
            accepted bool
          }

          api StartOrder {
            method POST
            path "/v1/orders/start"
            input StartOrderRequest
            output StartOrderResponse
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3406"),
        "validator should reject tenant-scoped API paths without tenant identity"
    );
}

void validator_rejects_invalid_api_server_declarations()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          api ExistingApi {
            method POST
            path "/v1/existing"
          }
          workflow NotAnApi {
            version 1
            start begin
            step begin {
              expected_execution_time PT1S
            }
          }
          api_server BrokenApiServer {
            serves MissingApi
            serves NotAnApi
            concurrency 0
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3002"),
        "validator should reject unknown or non-API api_server serves targets"
    );
    require(
        has_error_code(diagnostics, "SSPEC4002"),
        "validator should reject non-positive api_server concurrency"
    );
}

void validator_rejects_missing_required_declarations()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity EmptyEntity {
            key id
          }
          queue EmptyQueue {
          }
          lease EmptyLease {
          }
          workflow EmptyWorkflow {
          }
          api EmptyApi {
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4001"),
        "validator should reject missing required declarations"
    );
}

void validator_rejects_invalid_positive_and_non_negative_values()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          queue EmailQueue {
            namespace workflow_ns
            channel email
            visibility_timeout PT30S
            max_attempts 0
            message SendConfirmation {
              idempotency_key message_id
              payload { message_id string }
            }
          }
          workflow OrderProcessing {
            version 0
            start validate_order
            step validate_order {
              expected_execution_time PT10S
              max_retries -1
            }
          }
          worker OrderWorker {
            concurrency 0
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4002"),
        "validator should reject non-positive integer values"
    );
    require(
        has_error_code(diagnostics, "SSPEC4003"), "validator should reject negative integer values"
    );
}

void validator_rejects_incomplete_queue_canonical_shape()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          queue EmailQueue {
            namespace workflow_ns
            channel email
            message SendConfirmation {
              idempotency_key message_id
              payload { message_id string }
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "visibility_timeout"),
        "validator should require explicit queue visibility_timeout"
    );
    require(
        has_error_message_containing(diagnostics, "max_attempts"),
        "validator should require explicit queue max_attempts"
    );
}

void validator_rejects_incomplete_lease_canonical_shape()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          lease WorkerLease {
            resource "worker"
            ttl PT30S
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "renew_every"),
        "validator should require explicit lease renew_every"
    );
    require(
        has_error_message_containing(diagnostics, "holder"),
        "validator should require explicit lease holder"
    );
    require(
        has_error_message_containing(diagnostics, "fencing_token"),
        "validator should require explicit lease fencing_token"
    );
    require(
        has_error_message_containing(diagnostics, "max_ttl"),
        "validator should require explicit lease max_ttl"
    );
}

void validator_rejects_invalid_lease_timing()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          lease SlowRenewal {
            resource "worker"
            ttl PT30S
            renew_every PT30S
            holder worker_id
            fencing_token true
            max_ttl PT5M
          }

          lease MaxTooShort {
            resource "worker"
            ttl PT5M
            renew_every PT10S
            holder worker_id
            fencing_token true
            max_ttl PT30S
          }
        }
    )sspec");

    require(has_error_code(diagnostics, "SSPEC3501"), "validator should reject renew_every >= ttl");
    require(has_error_code(diagnostics, "SSPEC3502"), "validator should reject ttl > max_ttl");
}

void validator_accepts_feature_flags()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          feature_flag NewScheduler {
            type bool
            default false
            scope tenant
          }
          feature_flag MaxPendingOrders {
            type int
            default 100
            scope entity Order
          }
          shape StartOrderProcessingRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderProcessingResponse {
            accepted bool
          }
          entity Order {
            key tenant_id, order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              order_id string
            }
            state_machine {
              state Pending
              state Active
              state Failed {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Pending
              terminal Failed
              Pending -> Active
              Pending -> Failed
            }
          }
          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderProcessingRequest
            output StartOrderProcessingResponse
          }
          policy WorkflowAccess {
            tenant scoped_by tenant_id
            allow StartOrderProcessing when feature_enabled(NewScheduler);
            quota pending_orders: feature_value(MaxPendingOrders) >= 0;
          }
        }
    )sspec");

    require(!diagnostics.has_errors(), "validator should accept valid feature flags");
}

void validator_rejects_invalid_feature_flag_declarations()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          feature_flag new_scheduler {
            type bool
            default "false"
          }
          feature_flag MissingType {
            default false
          }
          feature_flag UnsupportedType {
            type json
            default "{}"
          }
          feature_flag MissingDefault {
            type bool
          }
          feature_flag ScopedToMissingEntity {
            type bool
            default false
            scope entity MissingEntity
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4201"),
        "validator should reject non-PascalCase feature flags"
    );
    require(
        has_error_code(diagnostics, "SSPEC4202"),
        "validator should reject unsupported feature flag types"
    );
    require(
        has_error_code(diagnostics, "SSPEC4203"), "validator should reject default/type mismatches"
    );
    require(
        has_error_code(diagnostics, "SSPEC4001"),
        "validator should reject missing feature flag type/default"
    );
    require(
        has_error_code(diagnostics, "SSPEC3002"),
        "validator should reject unknown feature flag entity scope"
    );
}

void validator_rejects_invalid_feature_flag_expression_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          feature_flag SchedulerMode {
            type string
            default "current"
          }
          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
          }
          policy WorkflowAccess {
            tenant scoped_by tenant_id
            allow StartOrderProcessing when feature_enabled(SchedulerMode);
            deny StartOrderProcessing when feature_value(MissingFlag) == "off";
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4204"),
        "validator should reject feature_enabled on non-bool flags"
    );
    require(
        has_error_code(diagnostics, "SSPEC3002"),
        "validator should reject unknown feature flag expression references"
    );
}

void validator_accepts_values_enums_and_events()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          value OrderAmount: decimal where OrderAmount >= 0;

          enum OrderStatus {
            Pending = "pending"
            Processing = "processing"
            Complete
          }

          event OrderAccepted {
            fields {
              order_id uuid
              amount OrderAmount
              status OrderStatus
            }
          }

          workflow AcceptOrder {
            version 1
            singleton false
            on OrderAccepted
            expected_execution_time PT30S
            start done
            step done {
              expected_execution_time PT1S
              max_retries 1
              emit OrderAccepted;
            }
          }
        }
    )sspec");

    require(!diagnostics.has_errors(), "validator should accept valid values, enums, and events");
}

void validator_rejects_invalid_values_enums_and_events()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          value orderAmount: MissingType;

          enum orderStatus {
            pending = "pending"
            pending = "duplicate"
          }

          enum EmptyStatus {
          }

          event orderAccepted {
            fields {
              order_id MissingType
              order_id string
            }
          }

          event EmptyEvent {
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4501"), "validator should reject invalid value names"
    );
    require(has_error_code(diagnostics, "SSPEC4601"), "validator should reject invalid enum names");
    require(
        has_error_code(diagnostics, "SSPEC4602"), "validator should reject invalid enum members"
    );
    require(
        has_error_code(diagnostics, "SSPEC4701"), "validator should reject invalid event names"
    );
    require(
        has_error_code(diagnostics, "SSPEC3001"),
        "validator should reject duplicate enum members and event fields"
    );
    require(
        has_error_code(diagnostics, "SSPEC3002"),
        "validator should reject unknown value and event field types"
    );
    require(
        has_error_code(diagnostics, "SSPEC4001"), "validator should reject empty enums and events"
    );
}

void validator_accepts_namespaces_external_systems_and_policies()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartInvoiceRequest {
            tenant_id string
            invoice_id string
          }
          shape StartInvoiceResponse {
            accepted bool
          }

          namespace Billing {
            external_system Stripe {
              owner: "payments"
              endpoint: "https://api.stripe.test"
            }

            api StartInvoice {
              method POST
              path "/v1/tenants/{tenantId}/invoices/start"
              input StartInvoiceRequest
              output StartInvoiceResponse
            }
          }

          policy BillingAccess {
            tenant scoped_by tenant_id
            allow Billing.StartInvoice when caller.role == billing_admin;
            quota starts_per_minute: 60;
            audit Billing.StartInvoice;
          }
        }
    )sspec");

    require(
        !diagnostics.has_errors(),
        "validator should accept valid namespaces, external systems, and policies"
    );
}

void validator_rejects_invalid_namespaces_and_external_systems()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          namespace billing {
            external_system stripe {
              owner: "payments"
              owner: "duplicate"
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4801"), "validator should reject invalid namespace names"
    );
    require(
        has_error_code(diagnostics, "SSPEC4901"),
        "validator should reject invalid external system names"
    );
    require(
        has_error_code(diagnostics, "SSPEC3001"),
        "validator should reject duplicate external system properties"
    );
}

void validator_accepts_logs_and_metrics()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          log WorkflowLaunchDecision {
            level info
            event_name "workflow.launch.decision"
            fields {
              tenant_id string
              decision string
            }
          }

          metric WorkflowLaunchAttempts {
            kind counter
            name "workflow_launch_attempts_total"
            unit count
            labels {
              tenant_id string
              decision string
              admitted bool
              retry_count int
            }
          }
        }
    )sspec");

    require(!diagnostics.has_errors(), "validator should accept valid logs and metrics");
}

void validator_rejects_missing_observability_tenant_fields()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          log WorkflowLaunchDecision {
            level info
            event_name "workflow.launch.decision"
            fields {
              workflow_name string
              decision string
            }
          }

          metric WorkflowLaunchAttempts {
            kind counter
            name "workflow_launch_attempts_total"
            unit count
            labels {
              workflow_name string
              decision string
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3407"),
        "validator should reject tenant-scoped logs without the tenant field"
    );
    require(
        has_error_code(diagnostics, "SSPEC3408"),
        "validator should reject tenant-scoped metrics without the tenant label"
    );
}

void validator_rejects_invalid_log_declarations()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          log workflowLaunchDecision {
            level trace
            fields {
              tenant_id MissingType
            }
          }

          log DuplicateEventA {
            level info
            event_name "workflow.launch"
          }

          log DuplicateEventB {
            level warn
            event_name "workflow.launch"
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4301"), "validator should reject non-PascalCase logs"
    );
    require(
        has_error_code(diagnostics, "SSPEC4302"), "validator should reject unsupported log levels"
    );
    require(
        has_error_code(diagnostics, "SSPEC4303"), "validator should reject duplicate log events"
    );
    require(
        has_error_code(diagnostics, "SSPEC4001"), "validator should reject missing log event_name"
    );
    require(
        has_error_code(diagnostics, "SSPEC3002"), "validator should reject unknown log field types"
    );
}

void validator_rejects_invalid_metric_declarations()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          metric workflowLaunchAttempts {
            kind timer
            labels {
              tenant_id uuid
              observed_at timestamp
              tenant_id string
            }
          }

          metric DuplicateMetricA {
            kind counter
            name "workflow_launch_total"
            unit count
          }

          metric DuplicateMetricB {
            kind gauge
            name "workflow_launch_total"
            unit count
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4401"), "validator should reject non-PascalCase metrics"
    );
    require(
        has_error_code(diagnostics, "SSPEC4402"), "validator should reject unsupported metric kinds"
    );
    require(
        has_error_code(diagnostics, "SSPEC4403"),
        "validator should reject duplicate metric backend names"
    );
    require(
        has_error_code(diagnostics, "SSPEC4404"),
        "validator should reject unsupported metric label types"
    );
    require(
        has_error_code(diagnostics, "SSPEC4001"), "validator should reject missing metric name/unit"
    );
    require(
        has_error_code(diagnostics, "SSPEC3001"), "validator should reject duplicate metric labels"
    );
}

void validator_rejects_high_cardinality_metric_labels()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          metric WorkflowLaunchAttempts {
            kind counter
            name "workflow_launch_attempts_total"
            unit count
            labels {
              tenant_id string
              order_id string
              workflow_name string
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4405"),
        "validator should reject high-cardinality metric labels"
    );
}

void validator_warns_on_noncanonical_observability_order()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          log WorkflowLaunchDecision {
            fields {
              tenant_id string
              decision string
            }
            event_name "workflow.launch.decision"
            level info
          }

          metric WorkflowLaunchAttempts {
            labels {
              tenant_id string
              decision string
            }
            unit count
            name "workflow_launch_attempts_total"
            kind counter
          }
        }
    )sspec");

    require(
        !diagnostics.has_errors(),
        "noncanonical observability order should warn without invalidating the spec"
    );
    require(has_warning_code(diagnostics, "SSPEC6104"), "validator should warn on log order");
    require(has_warning_code(diagnostics, "SSPEC6105"), "validator should warn on metric order");
}

void validator_warns_on_noncanonical_system_order()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          entity Order {
            key tenant_id, order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              order_id string
            }
            state_machine {
              state Active
              initial Active
            }
          }

          log WorkflowLaunchDecision {
            level info
            event_name "workflow.launch.decision"
            fields {
              tenant_id string
              decision string
            }
          }
        }
    )sspec");

    require(
        !diagnostics.has_errors(),
        "noncanonical system order should warn without invalidating the spec"
    );
    require(has_warning_code(diagnostics, "SSPEC6106"), "validator should warn on system order");
}

} // namespace

TEST_CASE("validator accepts resolved references")
{
    validator_accepts_resolved_references();
}

TEST_CASE("validator rejects duplicate top-level names")
{
    validator_rejects_duplicate_top_level_names();
}

TEST_CASE("validator rejects invalid shapes")
{
    validator_rejects_invalid_shapes();
}

TEST_CASE("validator warns on noncanonical entity and workflow order")
{
    validator_warns_on_noncanonical_entity_and_workflow_order();
}

TEST_CASE("validator rejects missing entity key fields")
{
    validator_rejects_missing_entity_key_field();
}

TEST_CASE("validator rejects invalid entity indexes")
{
    validator_rejects_invalid_entity_indexes();
}

TEST_CASE("validator rejects missing entity management model")
{
    validator_rejects_missing_entity_management_model();
}

TEST_CASE("validator rejects missing system tenancy")
{
    validator_rejects_missing_system_tenancy();
}

TEST_CASE("validator rejects missing tenant field propagation")
{
    validator_rejects_missing_tenant_field_propagation();
}

TEST_CASE("validator rejects policy tenant scope mismatch")
{
    validator_rejects_policy_tenant_scope_mismatch();
}

TEST_CASE("validator rejects incomplete policy canonical shape")
{
    validator_rejects_incomplete_policy_canonical_shape();
}

TEST_CASE("validator warns on noncanonical policy order")
{
    validator_warns_on_noncanonical_policy_order();
}

TEST_CASE("validator rejects invalid entity management field types")
{
    validator_rejects_invalid_entity_management_field_types();
}

TEST_CASE("validator rejects noncanonical entity management field order")
{
    validator_rejects_noncanonical_entity_management_field_order();
}

TEST_CASE("validator accepts terminal garbage collection modes")
{
    validator_accepts_terminal_garbage_collection_modes();
}

TEST_CASE("validator rejects invalid terminal garbage collection")
{
    validator_rejects_invalid_terminal_garbage_collection();
}

TEST_CASE("validator rejects implicit terminal retention")
{
    validator_rejects_implicit_terminal_retention();
}

TEST_CASE("validator rejects unknown workflow start steps")
{
    validator_rejects_unknown_workflow_start_step();
}

TEST_CASE("validator rejects incomplete workflow canonical shape")
{
    validator_rejects_incomplete_workflow_canonical_shape();
}

TEST_CASE("validator rejects invalid workflow behavior references")
{
    validator_rejects_invalid_workflow_behavior_references();
}

TEST_CASE("validator rejects unknown worker references")
{
    validator_rejects_unknown_worker_references();
}

TEST_CASE("validator rejects unknown API references")
{
    validator_rejects_unknown_api_references();
}

TEST_CASE("validator rejects APIs with multiple primary actions")
{
    validator_rejects_api_with_multiple_primary_actions();
}

TEST_CASE("validator rejects incomplete API canonical shape")
{
    validator_rejects_incomplete_api_canonical_shape();
}

TEST_CASE("validator rejects tenant-scoped API path without tenant identity")
{
    validator_rejects_tenant_scoped_api_path_without_tenant_identity();
}

TEST_CASE("validator rejects invalid API server declarations")
{
    validator_rejects_invalid_api_server_declarations();
}

TEST_CASE("validator rejects missing required declarations")
{
    validator_rejects_missing_required_declarations();
}

TEST_CASE("validator rejects invalid positive and non-negative values")
{
    validator_rejects_invalid_positive_and_non_negative_values();
}

TEST_CASE("validator rejects incomplete queue canonical shape")
{
    validator_rejects_incomplete_queue_canonical_shape();
}

TEST_CASE("validator rejects incomplete lease canonical shape")
{
    validator_rejects_incomplete_lease_canonical_shape();
}

TEST_CASE("validator rejects invalid lease timing")
{
    validator_rejects_invalid_lease_timing();
}

TEST_CASE("validator accepts feature flags")
{
    validator_accepts_feature_flags();
}

TEST_CASE("validator rejects invalid feature flag declarations")
{
    validator_rejects_invalid_feature_flag_declarations();
}

TEST_CASE("validator rejects invalid feature flag expression references")
{
    validator_rejects_invalid_feature_flag_expression_references();
}

TEST_CASE("validator accepts values, enums, and events")
{
    validator_accepts_values_enums_and_events();
}

TEST_CASE("validator rejects invalid values, enums, and events")
{
    validator_rejects_invalid_values_enums_and_events();
}

TEST_CASE("validator accepts namespaces, external systems, and policies")
{
    validator_accepts_namespaces_external_systems_and_policies();
}

TEST_CASE("validator rejects invalid namespaces and external systems")
{
    validator_rejects_invalid_namespaces_and_external_systems();
}

TEST_CASE("validator accepts logs and metrics")
{
    validator_accepts_logs_and_metrics();
}

TEST_CASE("validator rejects observability declarations missing tenant fields")
{
    validator_rejects_missing_observability_tenant_fields();
}

TEST_CASE("validator rejects invalid log declarations")
{
    validator_rejects_invalid_log_declarations();
}

TEST_CASE("validator rejects invalid metric declarations")
{
    validator_rejects_invalid_metric_declarations();
}

TEST_CASE("validator rejects high-cardinality metric labels")
{
    validator_rejects_high_cardinality_metric_labels();
}

TEST_CASE("validator warns on noncanonical observability order")
{
    validator_warns_on_noncanonical_observability_order();
}

TEST_CASE("validator warns on noncanonical system order")
{
    validator_warns_on_noncanonical_system_order();
}
