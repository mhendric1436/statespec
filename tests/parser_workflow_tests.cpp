#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"

namespace
{

void parser_parses_workflow_steps()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          workflow OrderProcessing {
            version 1
            singleton false
            expected_execution_time PT5M
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
              emit OrderStarted {
                order_id = order.order_id;
              };
              enqueue EmailDispatch.SendConfirmation {
                order_id = order.order_id;
              };
              acquire lease OrderWorkerLease as workerLease;
              renew lease OrderWorkerLease;
              release lease OrderWorkerLease;
              start workflow SendReceipt {
                order_id = order.order_id;
              };
              transition_to charge_payment;
            }
            step charge_payment {
              expected_execution_time PT30S
              max_retries 3
              complete;
            }
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(
        spec.system->workflows.size() == 1, "parser should parse one workflow"
    );
    const auto& workflow = spec.system->workflows[0];
    statespec::test::require(
        workflow.name == "OrderProcessing", "parser should parse workflow name"
    );
    statespec::test::require(workflow.version == 1, "parser should parse workflow version");
    statespec::test::require(workflow.singleton == false, "parser should parse singleton value");
    statespec::test::require(
        workflow.expected_execution_time == "PT5M", "parser should parse workflow duration"
    );
    statespec::test::require(
        workflow.start_step == "validate_order", "parser should parse start step"
    );
    statespec::test::require(workflow.on == "StartOrderProcessing", "parser should parse trigger");
    statespec::test::require(workflow.input == "StartOrderRequest", "parser should parse input");
    statespec::test::require(
        workflow.state == "OrderProcessingState", "parser should parse workflow state"
    );
    statespec::test::require(workflow.loads.size() == 1, "parser should parse workflow loads");
    statespec::test::require(
        workflow.loads[0].entity == "Order", "parser should parse workflow load entity"
    );
    statespec::test::require(
        workflow.loads[0].key_field == "order_id", "parser should parse workflow load key"
    );
    statespec::test::require(
        workflow.loads[0].binding == "order", "parser should parse workflow load binding"
    );
    statespec::test::require(workflow.steps.size() == 2, "parser should parse workflow steps");
    statespec::test::require(
        workflow.steps[0].name == "validate_order", "parser should parse first step name"
    );
    statespec::test::require(
        workflow.steps[0].expected_execution_time == "PT10S", "parser should parse step duration"
    );
    statespec::test::require(workflow.steps[0].max_retries == 2, "parser should parse max_retries");
    statespec::test::require(
        workflow.steps[0].statements.size() == 9, "parser should parse workflow statements"
    );
    statespec::test::require(
        workflow.steps[0].statements[0].kind == "require", "parser should parse require"
    );
    statespec::test::require(
        workflow.steps[0].statements[1].assignable == "order . status",
        "parser should parse set assignable"
    );
    statespec::test::require(
        workflow.steps[0].statements[3].target == "EmailDispatch.SendConfirmation",
        "parser should parse enqueue target"
    );
    statespec::test::require(
        workflow.steps[0].statements[3].payload.size() == 1, "parser should parse statement payload"
    );
    statespec::test::require(
        workflow.steps[0].statements[4].binding == "workerLease",
        "parser should parse acquire binding"
    );
    statespec::test::require(
        workflow.steps[0].statements[8].target == "charge_payment",
        "parser should parse transition target"
    );
    statespec::test::require(
        workflow.steps[1].statements.size() == 1 &&
            workflow.steps[1].statements[0].kind == "complete",
        "parser should parse complete statement"
    );
}

void parser_parses_nested_workflow_blocks_and_child_sets()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          workflow ParentWorkflow {
            version 1
            singleton false
            expected_execution_time PT5M
            start reserve_children
            child_set children_to_create {
              entity Child
              parent_field parent_id
              id_type uuid
              pending pending_child_ids
              creating creating_child_ids
              succeeded succeeded_child_ids
              failed failed_child_ids
              desired_count order.desired_child_count
            }
            step reserve_children {
              expected_execution_time PT10S
              max_retries 2
              atomic {
                reserve child_set children_to_create;
                when feature_enabled(NewScheduler) {
                  for_each child_id in order.pending_child_ids {
                    create child Child {
                      key = child_id;
                      parent = order.order_id;
                    }
                    observe child Child by child_id = child_id;
                    move child_id from order.pending_child_ids to order.creating_child_ids;
                  }
                }
                transition_to waiting_children;
              }
            }
            step waiting_children {
              expected_execution_time PT30S
              max_retries 3
              reconcile child_set children_to_create;
              complete;
            }
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    const auto& workflow = spec.system->workflows[0];
    statespec::test::require(
        workflow.child_sets.size() == 1, "parser should parse workflow child sets"
    );
    statespec::test::require(
        workflow.child_sets[0].desired_count == "order . desired_child_count",
        "parser should parse child_set desired_count expression"
    );
    const auto& atomic = workflow.steps[0].statements[0];
    statespec::test::require(atomic.kind == "atomic", "parser should parse atomic block");
    statespec::test::require(
        atomic.statements.size() == 3, "parser should preserve atomic nested statements"
    );
    statespec::test::require(
        atomic.statements[0].kind == "reserve_child_set",
        "parser should parse child_set reserve statement"
    );
    const auto& when = atomic.statements[1];
    statespec::test::require(when.kind == "when", "parser should parse when block");
    statespec::test::require(
        when.expression == "feature_enabled ( NewScheduler )", "parser should parse when expression"
    );
    const auto& for_each = when.statements[0];
    statespec::test::require(for_each.kind == "for_each", "parser should parse for_each block");
    statespec::test::require(
        for_each.binding == "child_id", "parser should parse for_each binding"
    );
    statespec::test::require(
        for_each.statements.size() == 3, "parser should preserve for_each nested statements"
    );
    statespec::test::require(
        for_each.statements[0].kind == "create_child", "parser should parse create child statement"
    );
    statespec::test::require(
        for_each.statements[0].payload.size() == 2,
        "parser should parse create child initialization"
    );
    statespec::test::require(
        for_each.statements[1].kind == "observe_child",
        "parser should parse observe child statement"
    );
    statespec::test::require(
        for_each.statements[2].from_assignable == "order . pending_child_ids",
        "parser should parse move from assignable"
    );
}

void parser_parses_child_workflow_blocks()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          workflow AccountLifecycle {
            version 1
            start generate_task_ids
            load Account by account_id as account
            child_workflow tasks {
              child_entity Task
              child_workflow TaskLifecycle
              child_id task_id uuid
              parent_ref account_id = account.account_id
              desired_count account.desired_task_count
              create {
                tenant_id: account.tenant_id
                account_id: account.account_id
                task_id: task_id
              }
              success when Task.status == "Active"
              failure when Task.status == "Failed"
            }
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(
        spec.system->workflows.size() == 1, "parser should parse one workflow"
    );
    const auto& workflow = spec.system->workflows[0];
    statespec::test::require(
        workflow.child_workflows.size() == 1, "parser should parse child_workflow blocks"
    );
    const auto& child_workflow = workflow.child_workflows[0];
    statespec::test::require(
        child_workflow.name == "tasks", "parser should parse child_workflow name"
    );
    statespec::test::require(
        child_workflow.child_entity == "Task", "parser should parse child entity"
    );
    statespec::test::require(
        child_workflow.child_workflow == "TaskLifecycle", "parser should parse child workflow"
    );
    statespec::test::require(
        child_workflow.child_id_field == "task_id", "parser should parse child id field"
    );
    statespec::test::require(
        child_workflow.child_id_type == "uuid", "parser should parse child id type"
    );
    statespec::test::require(
        child_workflow.parent_ref_field == "account_id", "parser should parse parent_ref field"
    );
    statespec::test::require(
        child_workflow.parent_ref_expression == "account . account_id",
        "parser should parse parent_ref expression"
    );
    statespec::test::require(
        child_workflow.desired_count == "account . desired_task_count",
        "parser should parse desired_count expression"
    );
    statespec::test::require(
        child_workflow.create_assignments.size() == 3, "parser should parse create assignments"
    );
    statespec::test::require(
        child_workflow.create_assignments[0].name == "tenant_id",
        "parser should parse create assignment name"
    );
    statespec::test::require(
        child_workflow.create_assignments[0].expression == "account . tenant_id",
        "parser should parse create assignment expression"
    );
    statespec::test::require(
        child_workflow.success_expression == "Task . status == \"Active\"",
        "parser should parse success predicate"
    );
    statespec::test::require(
        child_workflow.failure_expression == "Task . status == \"Failed\"",
        "parser should parse failure predicate"
    );
}
} // namespace

TEST_CASE("parser parses workflow steps")
{
    parser_parses_workflow_steps();
}

TEST_CASE("parser parses nested workflow blocks and child sets")
{
    parser_parses_nested_workflow_blocks_and_child_sets();
}

TEST_CASE("parser parses child workflow blocks")
{
    parser_parses_child_workflow_blocks();
}
