#include "validator_test_support.hpp"

namespace
{

void validator_accepts_logs_and_metrics()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          log WorkflowLaunchDecision {
            level info
            event_name "workflow.launch.decision"
            fields {
              tenant_id string
              decision string
            }
          }

          metric WorkflowLaunchAttempts {
            kind counter
            name "workflow_launch_attempts_total"
            unit count
            labels {
              tenant_id string
              decision string
              admitted bool
              retry_count int
            }
          }
        }
    )sspec");

    require(!diagnostics.has_errors(), "validator should accept valid logs and metrics");
}

void validator_rejects_missing_observability_tenant_fields()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          log WorkflowLaunchDecision {
            level info
            event_name "workflow.launch.decision"
            fields {
              workflow_name string
              decision string
            }
          }

          metric WorkflowLaunchAttempts {
            kind counter
            name "workflow_launch_attempts_total"
            unit count
            labels {
              workflow_name string
              decision string
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::TenantLogMissingTenantField),
        "validator should reject tenant-scoped logs without the tenant field"
    );
    require(
        has_error_code(diagnostics, dc::TenantMetricMissingTenantLabel),
        "validator should reject tenant-scoped metrics without the tenant label"
    );
}

void validator_rejects_invalid_log_declarations()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          log workflowLaunchDecision {
            level trace
            fields {
              tenant_id MissingType
            }
          }

          log DuplicateEventA {
            level info
            event_name "workflow.launch"
          }

          log DuplicateEventB {
            level warn
            event_name "workflow.launch"
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::LogInvalidName),
        "validator should reject non-PascalCase logs"
    );
    require(
        has_error_code(diagnostics, dc::LogInvalidLevel),
        "validator should reject unsupported log levels"
    );
    require(
        has_error_code(diagnostics, dc::LogDuplicateEventName),
        "validator should reject duplicate log events"
    );
    require(
        has_error_code(diagnostics, dc::RequiredDeclaration),
        "validator should reject missing log event_name"
    );
    require(
        has_error_code(diagnostics, dc::UnknownReference),
        "validator should reject unknown log field types"
    );
}

void validator_rejects_invalid_metric_declarations()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          metric workflowLaunchAttempts {
            kind timer
            labels {
              tenant_id uuid
              observed_at timestamp
              tenant_id string
            }
          }

          metric DuplicateMetricA {
            kind counter
            name "workflow_launch_total"
            unit count
          }

          metric DuplicateMetricB {
            kind gauge
            name "workflow_launch_total"
            unit count
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::MetricInvalidName),
        "validator should reject non-PascalCase metrics"
    );
    require(
        has_error_code(diagnostics, dc::MetricInvalidKind),
        "validator should reject unsupported metric kinds"
    );
    require(
        has_error_code(diagnostics, dc::MetricDuplicateBackendName),
        "validator should reject duplicate metric backend names"
    );
    require(
        has_error_code(diagnostics, dc::MetricInvalidLabelType),
        "validator should reject unsupported metric label types"
    );
    require(
        has_error_code(diagnostics, dc::RequiredDeclaration),
        "validator should reject missing metric name/unit"
    );
    require(
        has_error_code(diagnostics, dc::DuplicateDeclaration),
        "validator should reject duplicate metric labels"
    );
}

void validator_rejects_high_cardinality_metric_labels()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          metric WorkflowLaunchAttempts {
            kind counter
            name "workflow_launch_attempts_total"
            unit count
            labels {
              tenant_id string
              order_id string
              workflow_name string
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::MetricHighCardinalityLabel),
        "validator should reject high-cardinality metric labels"
    );
}

void validator_warns_on_noncanonical_observability_order()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          log WorkflowLaunchDecision {
            fields {
              tenant_id string
              decision string
            }
            event_name "workflow.launch.decision"
            level info
          }

          metric WorkflowLaunchAttempts {
            labels {
              tenant_id string
              decision string
            }
            unit count
            name "workflow_launch_attempts_total"
            kind counter
          }
        }
    )sspec");

    require(
        !diagnostics.has_errors(),
        "noncanonical observability order should warn without invalidating the spec"
    );
    require(
        has_warning_code(diagnostics, dc::NoncanonicalLogOrder),
        "validator should warn on log order"
    );
    require(
        has_warning_code(diagnostics, dc::NoncanonicalMetricOrder),
        "validator should warn on metric order"
    );
}

void validator_warns_on_noncanonical_system_order()
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
              state Active
              initial Active
            }
          }

          log WorkflowLaunchDecision {
            level info
            event_name "workflow.launch.decision"
            fields {
              tenant_id string
              decision string
            }
          }
        }
    )sspec");

    require(
        !diagnostics.has_errors(),
        "noncanonical system order should warn without invalidating the spec"
    );
    require(
        has_warning_code(diagnostics, dc::NoncanonicalSystemOrder),
        "validator should warn on system order"
    );
}

} // namespace

TEST_CASE("validator accepts logs and metrics")
{
    validator_accepts_logs_and_metrics();
}

TEST_CASE("validator rejects observability declarations missing tenant fields")
{
    validator_rejects_missing_observability_tenant_fields();
}

TEST_CASE("validator rejects invalid log declarations")
{
    validator_rejects_invalid_log_declarations();
}

TEST_CASE("validator rejects invalid metric declarations")
{
    validator_rejects_invalid_metric_declarations();
}

TEST_CASE("validator rejects high-cardinality metric labels")
{
    validator_rejects_high_cardinality_metric_labels();
}

TEST_CASE("validator warns on noncanonical observability order")
{
    validator_warns_on_noncanonical_observability_order();
}

TEST_CASE("validator warns on noncanonical system order")
{
    validator_warns_on_noncanonical_system_order();
}
