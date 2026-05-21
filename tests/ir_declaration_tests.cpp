#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/ir.hpp"
#include "statespec/semantic.hpp"

namespace
{

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
} // namespace

TEST_CASE("IR lowers shapes")
{
    ir_lowers_shapes();
}

TEST_CASE("IR lowers values, enums, and events")
{
    ir_lowers_values_enums_and_events();
}
