#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/ir.hpp"
#include "statespec/semantic.hpp"

#include <fstream>
#include <sstream>

namespace
{

void ir_lowers_terminal_garbage_collection_policy()
{
    const auto spec = statespec::test::parse_text(R"sspec(
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
              state Creating
              state Deleted {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Creating
              terminal [Deleted]
              Creating -> Deleted
            }
          }
        }
    )sspec");

    const auto ir = statespec::lower_to_ir(spec);

    statespec::test::require(ir.entities.size() == 1, "IR should lower entity");
    const auto& entity = ir.entities[0];
    statespec::test::require(entity.states.size() == 2, "IR should lower entity states");
    statespec::test::require(entity.initial_state == "Creating", "IR should lower initial state");
    statespec::test::require(
        entity.terminal_states.size() == 1, "IR should lower terminal state list"
    );

    const auto& deleted = entity.states[1];
    statespec::test::require(deleted.name == "Deleted", "IR should lower state name");
    statespec::test::require(deleted.terminal, "IR should lower terminal flag");
    statespec::test::require(
        deleted.garbage_collection.has_value(), "IR should lower garbage collection policy"
    );
    statespec::test::require(
        deleted.garbage_collection->after == "P30D", "IR should lower garbage collection duration"
    );
    statespec::test::require(
        deleted.garbage_collection->mode == "tombstone", "IR should lower garbage collection mode"
    );
}

void ir_lowers_logs_and_metrics()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          log WorkflowLaunchDecision {
            level info
            event_name "workflow.launch.decision"
            fields {
              tenant_id string
              reservation_id string
              decision string
            }
          }

          metric WorkflowLaunchAttempts {
            kind counter
            name "workflow_launch_attempts_total"
            unit count
            labels {
              tenant_id string
              workflow_name string
              decision string
            }
          }
        }
    )sspec");

    const auto ir = statespec::lower_to_ir(spec);

    statespec::test::require(ir.logs.size() == 1, "IR should lower logs");
    const auto& log = ir.logs[0];
    statespec::test::require(log.name == "WorkflowLaunchDecision", "IR should lower log name");
    statespec::test::require(log.level == "info", "IR should lower log level");
    statespec::test::require(
        log.event_name == "workflow.launch.decision", "IR should lower log event name"
    );
    statespec::test::require(log.fields.size() == 3, "IR should lower log fields");
    statespec::test::require(log.fields[0].name == "tenant_id", "IR should lower log field name");
    statespec::test::require(log.fields[0].type == "string", "IR should lower log field type");

    statespec::test::require(ir.metrics.size() == 1, "IR should lower metrics");
    const auto& metric = ir.metrics[0];
    statespec::test::require(
        metric.name == "WorkflowLaunchAttempts", "IR should lower metric name"
    );
    statespec::test::require(metric.kind == "counter", "IR should lower metric kind");
    statespec::test::require(
        metric.backend_name == "workflow_launch_attempts_total",
        "IR should lower metric backend name"
    );
    statespec::test::require(metric.unit == "count", "IR should lower metric unit");
    statespec::test::require(metric.labels.size() == 3, "IR should lower metric labels");
    statespec::test::require(
        metric.labels[1].name == "workflow_name", "IR should lower metric label name"
    );
    statespec::test::require(
        metric.labels[1].type == "string", "IR should lower metric label type"
    );
}

void ir_lowers_shapes()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          shape CreateOrderRequest {
            tenant_id string
            order_id string
            priority int?
          }

          shape CreateOrderResponse {
            accepted bool
          }

          api CreateOrder {
            method POST
            path "/v1/tenants/{tenantId}/orders"
            input CreateOrderRequest
            output CreateOrderResponse
          }
        }
    )sspec");

    const auto ir = statespec::lower_to_ir(spec);

    statespec::test::require(ir.shapes.size() == 2, "IR should lower shapes");
    statespec::test::require(
        ir.shapes[0].name == "CreateOrderRequest", "IR should lower shape name"
    );
    statespec::test::require(ir.shapes[0].fields.size() == 3, "IR should lower shape fields");
    statespec::test::require(
        ir.shapes[0].fields[2].type == "int?", "IR should lower optional shape field"
    );
    statespec::test::require(
        ir.apis[0].input == "CreateOrderRequest", "IR should lower API input shape reference"
    );
    statespec::test::require(
        ir.apis[0].output == "CreateOrderResponse", "IR should lower API output shape reference"
    );
}

void ir_lowers_values_enums_and_events()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
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
            on OrderAccepted
            expected_execution_time PT30S
            start done
            step done {
              expected_execution_time PT1S
              emit OrderAccepted;
            }
          }
        }
    )sspec");

    const auto ir = statespec::lower_to_ir(spec);

    statespec::test::require(ir.values.size() == 1, "IR should lower values");
    statespec::test::require(ir.values[0].name == "OrderAmount", "IR should lower value name");
    statespec::test::require(ir.values[0].type == "decimal", "IR should lower value type");
    statespec::test::require(
        ir.values[0].constraint.has_value(), "IR should lower value constraint"
    );

    statespec::test::require(ir.enums.size() == 1, "IR should lower enums");
    statespec::test::require(ir.enums[0].members.size() == 3, "IR should lower enum members");
    statespec::test::require(
        ir.enums[0].members[0].value == "pending", "IR should lower enum member values"
    );

    statespec::test::require(ir.events.size() == 1, "IR should lower events");
    statespec::test::require(ir.events[0].fields.size() == 3, "IR should lower event fields");
    statespec::test::require(
        ir.events[0].fields[2].type == "OrderStatus", "IR should lower event enum field type"
    );
    statespec::test::require(
        ir.workflows[0].on == "OrderAccepted", "IR should lower workflow event trigger"
    );
}

void ir_lowers_namespaces_external_systems_and_policy_descriptors()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          namespace Billing {
            external_system Stripe {
              owner: "payments"
              endpoint: "https://api.stripe.test"
              metadata {
                entity ExternalSystemEndpoint
                profile_field profile
                required_fields [base_url, auth_ref, timeout_ms]
              }
            }

            api StartInvoice {
              method POST
              path "/v1/tenants/{tenantId}/invoices/start"
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

    const auto ir = statespec::lower_to_ir(spec);

    statespec::test::require(ir.namespaces.size() == 1, "IR should lower namespaces");
    statespec::test::require(ir.namespaces[0].name == "Billing", "IR should lower namespace name");
    statespec::test::require(
        ir.namespaces[0].members.size() == 2, "IR should lower namespace members"
    );
    statespec::test::require(ir.external_systems.size() == 1, "IR should lower external systems");
    statespec::test::require(
        ir.external_systems[0].name == "Billing.Stripe",
        "IR should lower qualified external system name"
    );
    statespec::test::require(
        ir.external_systems[0].properties[0].name == "owner",
        "IR should lower external system properties"
    );
    statespec::test::require(
        ir.external_systems[0].metadata.has_value(), "IR should lower external system metadata"
    );
    statespec::test::require(
        ir.external_systems[0].metadata->entity == "ExternalSystemEndpoint",
        "IR should lower external system metadata entity"
    );
    statespec::test::require(
        ir.external_systems[0].metadata->required_fields.size() == 3,
        "IR should lower external system required metadata fields"
    );
    statespec::test::require(ir.apis[0].name == "Billing.StartInvoice", "IR should qualify APIs");
    statespec::test::require(ir.policies.size() == 1, "IR should lower policies");
    statespec::test::require(
        ir.policies[0].allows[0].action == "Billing.StartInvoice",
        "IR should lower policy action references"
    );
}

void ir_lowers_entity_relationship_metadata()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          entity Account {
            key account_id
            children {
              orders: Order by account_id
            }
            fields {
              account_id string
              created_at timestamp
              updated_at timestamp
              status string
            }
            state_machine {
              state Active
              initial Active
            }
          }

          entity Order {
            key order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            relations {
              parent account_id: ref<Account> {
                kind: composition
                on_parent_delete: block
                parent_must_be_in: [Active]
                unique_within_parent: [order_id]
              }
            }
            fields {
              order_id string
              account_id string
              created_at timestamp
              updated_at timestamp
              status string
            }
            invariants {
              valid_status: status != ""
            }
            state_machine {
              state Pending
              initial Pending
            }
          }
        }
    )sspec");

    const auto resolved = statespec::resolve_semantics(spec);
    const auto ir = statespec::lower_to_ir(resolved);

    statespec::test::require(ir.entities.size() == 2, "IR should lower entities");
    statespec::test::require(ir.entities[0].children.size() == 1, "IR should lower children");
    statespec::test::require(
        ir.entities[0].children[0].target_entity == "Order", "IR should lower child target"
    );

    const auto& order = ir.entities[1];
    statespec::test::require(order.ownership.has_value(), "IR should lower ownership");
    statespec::test::require(
        order.ownership->authority == "system", "IR should lower ownership authority"
    );
    statespec::test::require(order.relations.size() == 1, "IR should lower relations");
    statespec::test::require(
        order.relations[0].target == "Account", "IR should lower resolved relation target"
    );
    statespec::test::require(
        order.relations[0].relation_kind == "composition", "IR should lower relation options"
    );
    statespec::test::require(order.invariants.size() == 1, "IR should lower invariants");
    statespec::test::require(
        order.invariants[0].name == "valid_status", "IR should lower invariant name"
    );
}

void ir_lowers_system_runtime_contracts()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderRequest {
            tenant_id string
            order_id string
          }

          shape OrderProcessingState {
            attempt int
          }

          entity Order {
            key tenant_id, order_id
            fields {
              tenant_id string
              order_id string
              status string
              created_at timestamp
              updated_at timestamp
            }
            indexes {
              index by_tenant_status on tenant_id, status
              unique by_tenant_order on tenant_id, order_id
            }
            state_machine {
              state Pending
              state Active
              initial Pending
              Pending -> Active
            }
          }

          queue EmailDispatch {
            namespace workflow_ns
            channel email
            visibility_timeout PT30S
            max_attempts 3
            dead_letter EmailDispatch.SendFailed
            message SendConfirmation {
              idempotency_key message_id
              payload {
                message_id string
                order_id string
              }
            }
          }

          lease OrderReconcilerLease {
            resource "orders:reconciler"
            ttl PT30S
            renew_every PT10S
            holder worker_id
            fencing_token true
            max_ttl PT5M
          }

          worker OrderWorker {
            singleton true
            lease OrderReconcilerLease
            polls EmailDispatch.SendConfirmation
            executes OrderProcessing
            concurrency 4
          }

          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/{order_id}/start"
            input StartOrderRequest
            output StartOrderResponse
            error ErrorResponse
            starts workflow OrderProcessing
          }

          api_server OrderApi {
            serves StartOrderProcessing
            concurrency 16
          }

          workflow OrderProcessing {
            version 2
            singleton false
            expected_execution_time PT60S
            on StartOrderProcessing
            input StartOrderRequest
            state OrderProcessingState
            load Order by order_id as order
            start validate_order
            step validate_order {
              expected_execution_time PT10S
              max_retries 2
              require order.status == "Pending";
              set order.status = "Active";
              enqueue EmailDispatch.SendConfirmation {
                order_id = order.order_id;
              };
              acquire lease OrderReconcilerLease as leaseToken;
              start workflow OrderProcessing {
                order_id = order.order_id;
              };
              transition_to validate_order;
            }
          }

          policy WorkflowAccess {
            tenant scoped_by tenant_id
            allow StartOrderProcessing when caller.role == operator;
            deny StartOrderProcessing when caller.suspended == true;
            quota starts_per_minute: 60;
            audit StartOrderProcessing;
          }
        }
    )sspec");

    const auto ir = statespec::lower_to_ir(spec);

    statespec::test::require(ir.tenant_scope.has_value(), "IR should lower tenant scope");
    statespec::test::require(
        ir.tenant_scope->field_name == "tenant_id", "IR should lower tenant scope field"
    );
    statespec::test::require(ir.system_tenant.has_value(), "IR should lower system tenant");
    statespec::test::require(
        ir.system_tenant->source == "configured", "IR should lower system tenant source"
    );

    statespec::test::require(ir.entities.size() == 1, "IR should lower entities");
    const auto& entity = ir.entities[0];
    statespec::test::require(entity.indexes.size() == 2, "IR should lower entity indexes");
    statespec::test::require(
        entity.indexes[0].name == "by_tenant_status", "IR should lower index name"
    );
    statespec::test::require(!entity.indexes[0].unique, "IR should lower non-unique index");
    statespec::test::require(entity.indexes[1].unique, "IR should lower unique index");
    statespec::test::require(entity.transitions.size() == 1, "IR should lower state transitions");
    statespec::test::require(
        entity.transitions[0].from == "Pending" && entity.transitions[0].to == "Active",
        "IR should lower transition endpoints"
    );

    statespec::test::require(ir.queues.size() == 1, "IR should lower queues");
    const auto& queue = ir.queues[0];
    statespec::test::require(queue.namespace_name == "workflow_ns", "IR should lower namespace");
    statespec::test::require(queue.messages.size() == 1, "IR should lower queue messages");
    statespec::test::require(
        queue.messages[0].idempotency_key == "message_id", "IR should lower message idempotency key"
    );
    statespec::test::require(
        queue.messages[0].payload_fields.size() == 2, "IR should lower message payload fields"
    );

    statespec::test::require(ir.leases.size() == 1, "IR should lower leases");
    statespec::test::require(ir.leases[0].holder == "worker_id", "IR should lower lease holder");
    statespec::test::require(
        ir.leases[0].fencing_token == true, "IR should lower lease fencing token"
    );

    statespec::test::require(ir.workers.size() == 1, "IR should lower workers");
    const auto& worker = ir.workers[0];
    statespec::test::require(worker.name == "OrderWorker", "IR should lower worker name");
    statespec::test::require(worker.singleton == true, "IR should lower worker singleton");
    statespec::test::require(
        worker.polls == "EmailDispatch.SendConfirmation", "IR should lower worker polls"
    );
    statespec::test::require(
        worker.executes == "OrderProcessing", "IR should lower worker execution target"
    );
    statespec::test::require(worker.concurrency == 4, "IR should lower worker concurrency");

    statespec::test::require(ir.api_servers.size() == 1, "IR should lower API servers");
    const auto& api_server = ir.api_servers[0];
    statespec::test::require(api_server.name == "OrderApi", "IR should lower API server name");
    statespec::test::require(
        api_server.serves.size() == 1 && api_server.serves[0] == "StartOrderProcessing",
        "IR should lower API server served APIs"
    );
    statespec::test::require(
        api_server.concurrency == 16, "IR should lower API server concurrency"
    );

    statespec::test::require(ir.apis.size() == 1, "IR should lower APIs");
    const auto& api = ir.apis[0];
    statespec::test::require(api.method == "POST", "IR should lower API method");
    statespec::test::require(
        api.starts_workflow == "OrderProcessing", "IR should lower API workflow target"
    );
    statespec::test::require(!api.enqueues.has_value(), "IR should omit API queue target");

    statespec::test::require(ir.workflows.size() == 1, "IR should lower workflows");
    const auto& workflow = ir.workflows[0];
    statespec::test::require(workflow.steps.size() == 1, "IR should lower workflow steps");
    statespec::test::require(
        workflow.on == "StartOrderProcessing", "IR should lower workflow trigger"
    );
    statespec::test::require(
        workflow.input == "StartOrderRequest", "IR should lower workflow input"
    );
    statespec::test::require(
        workflow.state == "OrderProcessingState", "IR should lower workflow state"
    );
    statespec::test::require(workflow.loads.size() == 1, "IR should lower workflow loads");
    statespec::test::require(
        workflow.loads[0].entity == "Order", "IR should lower workflow load entity"
    );
    statespec::test::require(
        workflow.loads[0].key_field == "order_id", "IR should lower workflow load key"
    );
    statespec::test::require(
        workflow.loads[0].binding == "order", "IR should lower workflow load binding"
    );
    statespec::test::require(
        workflow.steps[0].statements.size() == 6, "IR should lower workflow statements"
    );
    statespec::test::require(
        workflow.steps[0].statements[0].kind == "require", "IR should lower require statement kind"
    );
    statespec::test::require(
        workflow.steps[0].statements[1].assignable == "order . status",
        "IR should lower set statement assignable"
    );
    statespec::test::require(
        workflow.steps[0].statements[2].target == "EmailDispatch.SendConfirmation",
        "IR should lower enqueue target"
    );
    statespec::test::require(
        workflow.steps[0].statements[2].payload.size() == 1, "IR should lower enqueue payload"
    );
    statespec::test::require(
        workflow.steps[0].statements[3].binding == "leaseToken", "IR should lower lease binding"
    );
    statespec::test::require(
        workflow.steps[0].statements[5].target == "validate_order",
        "IR should lower transition target"
    );

    statespec::test::require(ir.policies.size() == 1, "IR should lower policies");
    const auto& policy = ir.policies[0];
    statespec::test::require(
        policy.tenant_scoped_by == "tenant_id", "IR should lower policy tenant scope"
    );
    statespec::test::require(policy.allows.size() == 1, "IR should lower allow rules");
    statespec::test::require(policy.denies.size() == 1, "IR should lower deny rules");
    statespec::test::require(policy.quotas.size() == 1, "IR should lower quotas");
    statespec::test::require(policy.audits.size() == 1, "IR should lower audit actions");
    statespec::test::require(
        policy.allows[0].condition == "caller . role == operator", "IR should lower allow condition"
    );
}

std::string read_fixture(const std::string& path)
{
    std::ifstream input(path);
    statespec::test::require(input.good(), "fixture should be readable");
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void ir_lowers_kitchen_sink_fixture()
{
    const auto spec =
        statespec::test::parse_text(read_fixture("testdata/parity/kitchen-sink.sspec"));
    const auto resolved = statespec::resolve_semantics(spec);
    const auto ir = statespec::lower_to_ir(resolved);

    statespec::test::require(ir.name == "KitchenSink", "IR should lower fixture system name");
    statespec::test::require(ir.tenant_scope.has_value(), "IR should lower fixture tenant scope");
    statespec::test::require(ir.system_tenant.has_value(), "IR should lower fixture system tenant");
    statespec::test::require(ir.feature_flags.size() == 1, "IR should lower fixture feature flags");
    statespec::test::require(ir.logs.size() == 1, "IR should lower fixture logs");
    statespec::test::require(ir.metrics.size() == 1, "IR should lower fixture metrics");
    statespec::test::require(ir.entities.size() == 2, "IR should lower fixture entities");
    statespec::test::require(ir.queues.size() == 1, "IR should lower fixture queues");
    statespec::test::require(ir.leases.size() == 1, "IR should lower fixture leases");
    statespec::test::require(ir.workers.size() == 1, "IR should lower fixture workers");
    statespec::test::require(ir.api_servers.size() == 1, "IR should lower fixture API servers");
    statespec::test::require(ir.apis.size() == 5, "IR should lower fixture APIs");
    statespec::test::require(ir.workflows.size() == 1, "IR should lower fixture workflows");
    statespec::test::require(ir.policies.size() == 1, "IR should lower fixture policies");

    statespec::test::require(
        resolved.feature_flags[0].scope_target.has_value() &&
            resolved.feature_flags[0].scope_target->kind == statespec::SymbolKind::Entity,
        "semantic resolver should resolve fixture feature flag scope"
    );
    statespec::test::require(
        resolved.workers[0].polls.has_value() &&
            resolved.workers[0].polls->kind == statespec::SymbolKind::Message,
        "semantic resolver should resolve fixture worker poll target"
    );
    statespec::test::require(
        resolved.apis[0].starts_workflow.has_value() &&
            resolved.apis[0].starts_workflow->kind == statespec::SymbolKind::Workflow,
        "semantic resolver should resolve fixture API workflow target"
    );
    statespec::test::require(
        resolved.api_servers[0].serves[0].kind == statespec::SymbolKind::Api,
        "semantic resolver should resolve fixture API server served API"
    );
}

} // namespace

TEST_CASE("IR lowers terminal garbage collection policy")
{
    ir_lowers_terminal_garbage_collection_policy();
}

TEST_CASE("IR lowers logs and metrics")
{
    ir_lowers_logs_and_metrics();
}

TEST_CASE("IR lowers shapes")
{
    ir_lowers_shapes();
}

TEST_CASE("IR lowers values, enums, and events")
{
    ir_lowers_values_enums_and_events();
}

TEST_CASE("IR lowers namespaces, external systems, and policies")
{
    ir_lowers_namespaces_external_systems_and_policy_descriptors();
}

TEST_CASE("IR lowers entity relationship metadata")
{
    ir_lowers_entity_relationship_metadata();
}

TEST_CASE("IR lowers system runtime contracts")
{
    ir_lowers_system_runtime_contracts();
}

TEST_CASE("IR lowers kitchen sink parity fixture")
{
    ir_lowers_kitchen_sink_fixture();
}
