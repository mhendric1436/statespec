#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/ir.hpp"
#include "statespec/semantic.hpp"

namespace
{

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
} // namespace

TEST_CASE("IR lowers system runtime contracts")
{
    ir_lowers_system_runtime_contracts();
}
