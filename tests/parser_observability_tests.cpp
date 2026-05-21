#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"

namespace
{

void parser_parses_logs_and_metrics()
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

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(spec.system->logs.size() == 1, "parser should parse one log");
    const auto& log = spec.system->logs[0];
    statespec::test::require(log.name == "WorkflowLaunchDecision", "parser should parse log name");
    statespec::test::require(log.level == "info", "parser should parse log level");
    statespec::test::require(
        log.event_name == "workflow.launch.decision", "parser should parse log event name"
    );
    statespec::test::require(log.fields.size() == 3, "parser should parse log fields");
    statespec::test::require(
        log.fields[0].name == "tenant_id", "parser should parse log field name"
    );
    statespec::test::require(log.fields[0].type == "string", "parser should parse log field type");
    statespec::test::require(log.member_order.size() == 3, "parser should track log member order");
    statespec::test::require(
        log.member_order[0].kind == "level", "parser should track first log member"
    );

    statespec::test::require(spec.system->metrics.size() == 1, "parser should parse one metric");
    const auto& metric = spec.system->metrics[0];
    statespec::test::require(
        metric.name == "WorkflowLaunchAttempts", "parser should parse metric name"
    );
    statespec::test::require(metric.kind == "counter", "parser should parse metric kind");
    statespec::test::require(
        metric.backend_name == "workflow_launch_attempts_total",
        "parser should parse metric backend name"
    );
    statespec::test::require(metric.unit == "count", "parser should parse metric unit");
    statespec::test::require(metric.labels.size() == 3, "parser should parse metric labels");
    statespec::test::require(
        metric.labels[1].name == "workflow_name", "parser should parse metric label name"
    );
    statespec::test::require(
        metric.labels[1].type == "string", "parser should parse metric label type"
    );
    statespec::test::require(
        metric.member_order.size() == 4, "parser should track metric member order"
    );
    statespec::test::require(
        metric.member_order[0].kind == "kind", "parser should track first metric member"
    );
}
} // namespace

TEST_CASE("parser parses logs and metrics")
{
    parser_parses_logs_and_metrics();
}
