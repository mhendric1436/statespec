#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/ir.hpp"
#include "statespec/semantic.hpp"

namespace
{

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
        log.level_kind == statespec::IrLogLevel::Info, "IR should normalize log level"
    );
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
        metric.kind_value == statespec::IrMetricKind::Counter, "IR should normalize metric kind"
    );
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

void ir_normalizes_feature_flag_descriptor_values()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          entity Order {
            key tenant_id, order_id
            fields {
              tenant_id string
              order_id string
              status string
              created_at timestamp
              updated_at timestamp
            }
            state_machine {
              state Pending
              state Active
              initial Pending
              Pending -> Active
            }
          }

          feature_flag OrderRollout {
            type decimal
            default 0.25
            scope entity Order
          }
        }
    )sspec");

    const auto ir = statespec::lower_to_ir(spec);

    statespec::test::require(ir.feature_flags.size() == 1, "IR should lower feature flags");
    const auto& flag = ir.feature_flags[0];
    statespec::test::require(
        flag.flag_type == statespec::IrFeatureFlagType::Decimal,
        "IR should normalize feature flag type"
    );
    statespec::test::require(
        flag.scope_kind == statespec::IrFeatureFlagScopeKind::Entity,
        "IR should normalize feature flag scope"
    );
    statespec::test::require(
        flag.default_value == "0.25", "IR should preserve feature flag default literal"
    );
}
} // namespace

TEST_CASE("IR lowers logs and metrics")
{
    ir_lowers_logs_and_metrics();
}

TEST_CASE("IR normalizes feature flag descriptor values")
{
    ir_normalizes_feature_flag_descriptor_values();
}
