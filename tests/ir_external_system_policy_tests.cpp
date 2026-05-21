#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/ir.hpp"
#include "statespec/semantic.hpp"

namespace
{

void ir_lowers_external_systems_and_policy_descriptors()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id

          entity ExternalSystemEndpoint {
            key tenant_id, external_system_id, profile
            fields {
              tenant_id string
              external_system_id string
              profile string
              base_url string
              auth_ref string
              timeout_ms int
            }
            state_machine {
              state Active
              initial Active
            }
          }

          external_system Stripe {
            owner: "payments"
            endpoint: "https://api.stripe.test"
            metadata {
              entity ExternalSystemEndpoint
              profile_field profile
              required_fields [base_url, auth_ref, timeout_ms]
              mappings {
                metadata.base_url -> client.base_url
                metadata.auth_ref -> client.auth_ref
                input.invoice_id -> request.invoice_id
              }
            }
          }

          api StartInvoice {
            method POST
            path "/v1/tenants/{tenantId}/invoices/start"
          }

          policy BillingAccess {
            tenant scoped_by tenant_id
            allow StartInvoice when caller.role == billing_admin;
            quota starts_per_minute: 60;
            audit StartInvoice;
          }
        }
    )sspec");

    const auto ir = statespec::lower_to_ir(spec);

    statespec::test::require(ir.external_systems.size() == 1, "IR should lower external systems");
    statespec::test::require(
        ir.external_systems[0].name == "Stripe", "IR should lower external system name"
    );
    statespec::test::require(
        ir.external_systems[0].properties[0].name == "owner",
        "IR should lower external system properties"
    );
    statespec::test::require(
        ir.external_systems[0].metadata.has_value(), "IR should lower external system metadata"
    );
    statespec::test::require(
        ir.external_systems[0].metadata->entity == "ExternalSystemEndpoint",
        "IR should lower external system metadata entity"
    );
    statespec::test::require(
        ir.external_systems[0].metadata->tenant_field == "tenant_id",
        "IR should lower external system metadata tenant field"
    );
    statespec::test::require(
        ir.external_systems[0].metadata->key_fields.size() == 3,
        "IR should lower external system metadata key fields"
    );
    statespec::test::require(
        ir.external_systems[0].metadata->key_fields[0] == "tenant_id",
        "IR should preserve metadata entity key field order"
    );
    statespec::test::require(
        ir.external_systems[0].metadata->required_fields.size() == 3,
        "IR should lower external system required metadata fields"
    );
    statespec::test::require(
        ir.external_systems[0].metadata->mappings.size() == 3,
        "IR should lower external system metadata mappings"
    );
    statespec::test::require(
        ir.external_systems[0].metadata->mappings[0].source == "metadata.base_url",
        "IR should lower external system metadata mapping source"
    );
    statespec::test::require(
        ir.external_systems[0].metadata->mappings[0].target == "client.base_url",
        "IR should lower external system metadata mapping target"
    );
    statespec::test::require(ir.apis[0].name == "StartInvoice", "IR should lower APIs");
    statespec::test::require(ir.policies.size() == 1, "IR should lower policies");
    statespec::test::require(
        ir.policies[0].allows[0].action == "StartInvoice",
        "IR should lower policy action references"
    );
}
} // namespace

TEST_CASE("IR lowers external systems and policies")
{
    ir_lowers_external_systems_and_policy_descriptors();
}
