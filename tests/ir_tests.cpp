#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/ir.hpp"

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

} // namespace

TEST_CASE("IR lowers terminal garbage collection policy")
{
    ir_lowers_terminal_garbage_collection_policy();
}

TEST_CASE("IR lowers logs and metrics")
{
    ir_lowers_logs_and_metrics();
}
