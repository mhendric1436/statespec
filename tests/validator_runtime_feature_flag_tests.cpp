#include "validator_test_support.hpp"

namespace
{

void validator_rejects_missing_required_declarations()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity EmptyEntity {
            key id
          }
          queue EmptyQueue {
          }
          lease EmptyLease {
          }
          workflow EmptyWorkflow {
          }
          api EmptyApi {
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::RequiredDeclaration),
        "validator should reject missing required declarations"
    );
}

void validator_rejects_invalid_positive_and_non_negative_values()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          queue EmailQueue {
            namespace workflow_ns
            channel email
            visibility_timeout PT30S
            max_attempts 0
            message SendConfirmation {
              idempotency_key message_id
              payload { message_id string }
            }
          }
          workflow OrderProcessing {
            version 0
            start validate_order
            step validate_order {
              expected_execution_time PT10S
              max_retries -1
            }
          }
          worker OrderWorker {
            concurrency 0
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::PositiveIntegerRequired),
        "validator should reject non-positive integer values"
    );
    require(
        has_error_code(diagnostics, dc::NonNegativeIntegerRequired),
        "validator should reject negative integer values"
    );
}

void validator_rejects_incomplete_queue_canonical_shape()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          queue EmailQueue {
            namespace workflow_ns
            channel email
            message SendConfirmation {
              idempotency_key message_id
              payload { message_id string }
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "visibility_timeout"),
        "validator should require explicit queue visibility_timeout"
    );
    require(
        has_error_message_containing(diagnostics, "max_attempts"),
        "validator should require explicit queue max_attempts"
    );
}

void validator_rejects_incomplete_lease_canonical_shape()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          lease WorkerLease {
            resource "worker"
            ttl PT30S
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "renew_every"),
        "validator should require explicit lease renew_every"
    );
    require(
        has_error_message_containing(diagnostics, "holder"),
        "validator should require explicit lease holder"
    );
    require(
        has_error_message_containing(diagnostics, "fencing_token"),
        "validator should require explicit lease fencing_token"
    );
    require(
        has_error_message_containing(diagnostics, "max_ttl"),
        "validator should require explicit lease max_ttl"
    );
}

void validator_rejects_invalid_lease_timing()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          lease SlowRenewal {
            resource "worker"
            ttl PT30S
            renew_every PT30S
            holder worker_id
            fencing_token true
            max_ttl PT5M
          }

          lease MaxTooShort {
            resource "worker"
            ttl PT5M
            renew_every PT10S
            holder worker_id
            fencing_token true
            max_ttl PT30S
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::LeaseRenewEveryTooLong),
        "validator should reject renew_every >= ttl"
    );
    require(
        has_error_code(diagnostics, dc::LeaseTtlTooLong), "validator should reject ttl > max_ttl"
    );
}

void validator_accepts_feature_flags()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          feature_flag NewScheduler {
            type bool
            default false
            scope tenant
          }
          feature_flag MaxPendingOrders {
            type int
            default 100
            scope entity Order
          }
          shape StartOrderProcessingRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderProcessingResponse {
            accepted bool
          }
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
              state Pending
              state Active
              state Failed {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Pending
              terminal Failed
              Pending -> Active
              Pending -> Failed
            }
          }
          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderProcessingRequest
            output StartOrderProcessingResponse
          }
          policy WorkflowAccess {
            tenant scoped_by tenant_id
            allow StartOrderProcessing when feature_enabled(NewScheduler) && contains(["operator", "admin"], caller.role);
            deny StartOrderProcessing when empty(caller.denied_permissions);
            quota pending_orders: feature_value(MaxPendingOrders) >= count(order.pending_ids);
          }
        }
    )sspec");

    require(!diagnostics.has_errors(), "validator should accept valid feature flags");
}

void validator_rejects_invalid_expressions()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          value BrokenValue: int where count(;

          shape StartOrderProcessingRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderProcessingResponse {
            accepted bool
          }
          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderProcessingRequest
            output StartOrderProcessingResponse
          }
          policy WorkflowAccess {
            tenant scoped_by tenant_id
            allow StartOrderProcessing when random(caller.role);
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::InvalidExpression),
        "validator should reject invalid expression syntax"
    );
    require(
        has_error_code(diagnostics, dc::ExpressionFunctionNotAllowed),
        "validator should reject unsupported expression functions"
    );
}

void validator_rejects_invalid_feature_flag_declarations()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          feature_flag new_scheduler {
            type bool
            default "false"
          }
          feature_flag MissingType {
            default false
          }
          feature_flag UnsupportedType {
            type json
            default "{}"
          }
          feature_flag MissingDefault {
            type bool
          }
          feature_flag ScopedToMissingEntity {
            type bool
            default false
            scope entity MissingEntity
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::FeatureFlagInvalidName),
        "validator should reject non-PascalCase feature flags"
    );
    require(
        has_error_code(diagnostics, dc::FeatureFlagInvalidType),
        "validator should reject unsupported feature flag types"
    );
    require(
        has_error_code(diagnostics, dc::FeatureFlagInvalidDefault),
        "validator should reject default/type mismatches"
    );
    require(
        has_error_code(diagnostics, dc::RequiredDeclaration),
        "validator should reject missing feature flag type/default"
    );
    require(
        has_error_code(diagnostics, dc::UnknownReference),
        "validator should reject unknown feature flag entity scope"
    );
}

void validator_rejects_invalid_feature_flag_expression_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          feature_flag SchedulerMode {
            type string
            default "current"
          }
          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
          }
          policy WorkflowAccess {
            tenant scoped_by tenant_id
            allow StartOrderProcessing when feature_enabled(SchedulerMode);
            deny StartOrderProcessing when feature_value(MissingFlag) == "off";
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::FeatureEnabledRequiresBoolFlag),
        "validator should reject feature_enabled on non-bool flags"
    );
    require(
        has_error_code(diagnostics, dc::UnknownReference),
        "validator should reject unknown feature flag expression references"
    );
}

} // namespace

TEST_CASE("validator rejects missing required declarations")
{
    validator_rejects_missing_required_declarations();
}

TEST_CASE("validator rejects invalid positive and non-negative values")
{
    validator_rejects_invalid_positive_and_non_negative_values();
}

TEST_CASE("validator rejects incomplete queue canonical shape")
{
    validator_rejects_incomplete_queue_canonical_shape();
}

TEST_CASE("validator rejects incomplete lease canonical shape")
{
    validator_rejects_incomplete_lease_canonical_shape();
}

TEST_CASE("validator rejects invalid lease timing")
{
    validator_rejects_invalid_lease_timing();
}

TEST_CASE("validator accepts feature flags")
{
    validator_accepts_feature_flags();
}

TEST_CASE("validator rejects invalid expressions")
{
    validator_rejects_invalid_expressions();
}

TEST_CASE("validator rejects invalid feature flag declarations")
{
    validator_rejects_invalid_feature_flag_declarations();
}

TEST_CASE("validator rejects invalid feature flag expression references")
{
    validator_rejects_invalid_feature_flag_expression_references();
}
