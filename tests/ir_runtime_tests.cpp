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
            child_set children_to_create {
              entity Order
              parent_field order_id
              desired_count 1
            }
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
              atomic {
                when order.status == "Active" {
                  transition_to validate_order;
                }
              }
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
    statespec::test::require(
        ir.leases[0].resource_pattern == "orders:reconciler",
        "IR should normalize lease resource pattern"
    );
    statespec::test::require(ir.leases[0].ttl_seconds == 30, "IR should normalize lease ttl");
    statespec::test::require(
        ir.leases[0].renew_every_seconds == 10, "IR should normalize lease renewal interval"
    );
    statespec::test::require(
        ir.leases[0].fencing_token_enabled, "IR should normalize lease fencing token"
    );
    statespec::test::require(
        ir.leases[0].max_ttl_seconds == 300, "IR should normalize lease max ttl"
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
        workflow.child_sets.size() == 1, "IR should lower workflow child sets"
    );
    statespec::test::require(
        workflow.child_sets[0].name == "children_to_create",
        "IR should lower workflow child_set name"
    );
    statespec::test::require(
        workflow.child_sets[0].desired_count == "1",
        "IR should lower workflow child_set desired_count"
    );
    statespec::test::require(
        workflow.steps[0].statements.size() == 7, "IR should lower workflow statements"
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
        workflow.steps[0].statements[5].kind == "atomic", "IR should lower atomic statement kind"
    );
    statespec::test::require(
        workflow.steps[0].statements[5].statements.size() == 1,
        "IR should lower nested atomic statements"
    );
    statespec::test::require(
        workflow.steps[0].statements[5].statements[0].kind == "when",
        "IR should lower nested when statement kind"
    );
    statespec::test::require(
        workflow.steps[0].statements[5].statements[0].statements[0].target == "validate_order",
        "IR should lower deeply nested transition target"
    );
    statespec::test::require(
        workflow.steps[0].statements[6].target == "validate_order",
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

void ir_lowers_workflow_child_workflows()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system ChildWorkflowSystem {
          entity Account {
            key account_id
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              account_id uuid
              desired_task_count int
            }
          }

          entity Task {
            key task_id
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              task_id uuid
              account_id uuid
            }
          }

          workflow TaskLifecycle {
            version 1
            singleton false
            expected_execution_time PT1M
            start run_task
            step run_task {
              expected_execution_time PT10S
              max_retries 1
            }
          }

          workflow AccountLifecycle {
            version 1
            singleton false
            expected_execution_time PT5M
            start run_account
            child_workflow tasks {
              child_entity Task
              child_workflow TaskLifecycle
              child_id task_id uuid
              parent_ref account_id = account.account_id
              desired_count account.desired_task_count
              create {
                task_id: task_id
                account_id: account.account_id
              }
              success when Task.status == Active
              failure when Task.status == Failed
            }
            step run_account {
              expected_execution_time PT10S
              max_retries 1
            }
          }
        }
    )sspec");

    const auto ir = statespec::lower_to_ir(spec);

    const auto& workflow = ir.workflows[1];
    statespec::test::require(
        workflow.child_workflows.size() == 1, "IR should lower workflow child_workflow blocks"
    );
    const auto& child_workflow = workflow.child_workflows[0];
    statespec::test::require(child_workflow.name == "tasks", "IR should lower child workflow name");
    statespec::test::require(
        child_workflow.child_entity == "Task", "IR should lower child workflow entity"
    );
    statespec::test::require(
        child_workflow.child_workflow == "TaskLifecycle", "IR should lower child workflow target"
    );
    statespec::test::require(
        child_workflow.child_id_field == "task_id", "IR should lower child id field"
    );
    statespec::test::require(
        child_workflow.child_id_type == "uuid", "IR should lower child id type"
    );
    statespec::test::require(
        child_workflow.parent_ref_field == "account_id", "IR should lower parent ref field"
    );
    statespec::test::require(
        child_workflow.parent_ref_expression == "account . account_id",
        "IR should lower parent ref expression"
    );
    statespec::test::require(
        child_workflow.desired_count == "account . desired_task_count",
        "IR should lower desired count expression"
    );
    statespec::test::require(
        child_workflow.create_assignments.size() == 2,
        "IR should lower child_workflow create assignments"
    );
    statespec::test::require(
        child_workflow.create_assignments[0].name == "task_id",
        "IR should lower child create assignment field"
    );
    statespec::test::require(
        child_workflow.success_expression == "Task . status == Active",
        "IR should lower child workflow success expression"
    );
    statespec::test::require(
        child_workflow.failure_expression == "Task . status == Failed",
        "IR should lower child workflow failure expression"
    );
    statespec::test::require(
        child_workflow.pending_bucket == "pending_task_ids",
        "IR should lower derived pending bucket"
    );
    statespec::test::require(
        child_workflow.creating_bucket == "creating_task_ids",
        "IR should lower derived creating bucket"
    );
    statespec::test::require(
        child_workflow.succeeded_bucket == "succeeded_task_ids",
        "IR should lower derived succeeded bucket"
    );
    statespec::test::require(
        child_workflow.failed_bucket == "failed_task_ids", "IR should lower derived failed bucket"
    );
    statespec::test::require(
        child_workflow.generate_ids_step == "generate_task_ids",
        "IR should lower derived generate ids step"
    );
    statespec::test::require(
        child_workflow.create_children_step == "create_tasks",
        "IR should lower derived create children step"
    );
    statespec::test::require(
        child_workflow.wait_children_step == "wait_for_tasks",
        "IR should lower derived wait children step"
    );
}
} // namespace

TEST_CASE("IR lowers system runtime contracts")
{
    ir_lowers_system_runtime_contracts();
}

TEST_CASE("IR lowers workflow child workflows")
{
    ir_lowers_workflow_child_workflows();
}
