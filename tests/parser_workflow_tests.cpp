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
} // namespace

TEST_CASE("parser parses workflow steps")
{
    parser_parses_workflow_steps();
}
