#include "validator_test_support.hpp"

namespace
{

void validator_rejects_missing_system_tenancy()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              order_id string
            }
            state_machine {
              state Active
              initial Active
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "tenant scoped_by"),
        "validator should require system tenant scope"
    );
    require(
        has_error_message_containing(diagnostics, "system_tenant configured"),
        "validator should require system tenant configuration"
    );
}

void validator_rejects_missing_tenant_field_propagation()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderRequest {
            order_id string
          }

          entity Order {
            key order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              order_id string
            }
            state_machine {
              state Pending
              initial Pending
            }
          }

          queue OrderQueue {
            namespace workflow_ns
            channel orders
            message StartOrder {
              idempotency_key message_id
              payload {
                message_id string
                order_id string
              }
            }
          }

          api StartOrder {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderRequest
            enqueues OrderQueue.StartOrder
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::TenantEntityMissingTenantField),
        "validator should require tenant field on tenant-scoped entities"
    );
    require(
        has_error_code(diagnostics, dc::TenantQueueMessageMissingTenantField),
        "validator should require tenant field on tenant-scoped queue messages"
    );
    require(
        has_error_code(diagnostics, dc::TenantApiInputMissingTenantField),
        "validator should require tenant field on tenant-scoped API input shapes"
    );
    require(
        has_error_code(diagnostics, dc::TenantEntityKeyMissingTenantField),
        "validator should require tenant field in tenant-scoped entity keys"
    );
}

void validator_rejects_policy_tenant_scope_mismatch()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderResponse {
            accepted bool
          }

          api StartOrder {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderRequest
            output StartOrderResponse
          }

          policy OrderAccess {
            tenant scoped_by account_id
            allow StartOrder when caller.role == operator;
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::TenantPolicyScopeMismatch),
        "validator should reject policy tenant scopes that differ from the system tenant scope"
    );
}

void validator_rejects_incomplete_policy_canonical_shape()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          policy EmptyAccess {
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "tenant scoped_by"),
        "validator should require explicit policy tenant scope"
    );
    require(
        has_error_message_containing(diagnostics, "at least one allow, deny, quota, or audit"),
        "validator should require at least one policy action"
    );
}

void validator_warns_on_noncanonical_policy_order()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderResponse {
            accepted bool
          }

          api StartOrder {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderRequest
            output StartOrderResponse
          }

          policy OrderAccess {
            tenant scoped_by tenant_id
            audit StartOrder;
            allow StartOrder when caller.role == operator;
          }
        }
    )sspec");

    require(!diagnostics.has_errors(), "noncanonical policy order should remain valid");
    require(
        has_warning_code(diagnostics, dc::NoncanonicalPolicyOrder),
        "validator should warn on policy order"
    );
}

} // namespace

TEST_CASE("validator rejects missing system tenancy")
{
    validator_rejects_missing_system_tenancy();
}

TEST_CASE("validator rejects missing tenant field propagation")
{
    validator_rejects_missing_tenant_field_propagation();
}

TEST_CASE("validator rejects policy tenant scope mismatch")
{
    validator_rejects_policy_tenant_scope_mismatch();
}

TEST_CASE("validator rejects incomplete policy canonical shape")
{
    validator_rejects_incomplete_policy_canonical_shape();
}

TEST_CASE("validator warns on noncanonical policy order")
{
    validator_warns_on_noncanonical_policy_order();
}
