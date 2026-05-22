#include "validator_test_support.hpp"

namespace
{

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
    require(
        has_warning_code(diagnostics, dc::NoncanonicalEntityOrder),
        "validator should warn on entity order"
    );
    require(
        has_warning_code(diagnostics, dc::NoncanonicalWorkflowOrder),
        "validator should warn on workflow order"
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
        has_error_code(diagnostics, dc::DuplicateDeclaration),
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
        has_error_code(diagnostics, dc::UnknownReference),
        "validator should reject unknown entity key field"
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
        has_error_code(diagnostics, dc::DuplicateDeclaration),
        "validator should reject duplicate entity index names"
    );
    require(
        has_error_code(diagnostics, dc::UnknownReference),
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
        has_error_code(diagnostics, dc::EntityInvalidStateType),
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
        has_error_code(diagnostics, dc::EntityDuplicateFieldName),
        "validator should reject noncanonical entity management field order"
    );
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
        has_error_code(diagnostics, dc::EntityTerminalGcRequiresRetention),
        "validator should reject garbage collection on non-terminal states"
    );
    require(
        has_error_code(diagnostics, dc::RequiredDeclaration),
        "validator should reject missing garbage collection fields"
    );
    require(
        has_error_code(diagnostics, dc::DurationRequired),
        "validator should reject invalid garbage collection durations"
    );
    require(
        has_error_code(diagnostics, dc::EntityGcRequiresTerminalState),
        "validator should reject invalid garbage collection modes"
    );
    require(
        has_error_code(diagnostics, dc::DuplicateDeclaration),
        "validator should reject duplicate garbage collection policy blocks"
    );
    require(
        has_error_code(diagnostics, dc::EntityUnknownTransitionState),
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
        has_error_code(diagnostics, dc::EntityDuplicateState),
        "validator should require inline terminal states in the terminal list"
    );
    require(
        has_error_code(diagnostics, dc::EntityMissingInitialState),
        "validator should require terminal list states to declare terminal: true"
    );
    require(
        has_error_code(diagnostics, dc::EntityStateTransitionGcConflict),
        "validator should require terminal states to declare garbage_collection"
    );
}

} // namespace

TEST_CASE("validator accepts resolved references")
{
    validator_accepts_resolved_references();
}

TEST_CASE("validator warns on noncanonical entity and workflow order")
{
    validator_warns_on_noncanonical_entity_and_workflow_order();
}

TEST_CASE("validator rejects duplicate top-level names")
{
    validator_rejects_duplicate_top_level_names();
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
