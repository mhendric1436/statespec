#include "validator_test_support.hpp"

namespace
{

void validator_rejects_invalid_shapes()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          shape createOrderRequest {
            order_id MissingType
            order_id string
          }

          shape EmptyShape {
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3201"), "validator should reject non-PascalCase shapes"
    );
    require(
        has_error_code(diagnostics, "SSPEC3002"),
        "validator should reject unknown shape field types"
    );
    require(
        has_error_code(diagnostics, "SSPEC3001"), "validator should reject duplicate shape fields"
    );
    require(has_error_code(diagnostics, "SSPEC4001"), "validator should reject empty shapes");
}

void validator_accepts_values_enums_and_events()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

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
            singleton false
            on OrderAccepted
            expected_execution_time PT30S
            start done
            step done {
              expected_execution_time PT1S
              max_retries 1
              emit OrderAccepted;
            }
          }
        }
    )sspec");

    require(!diagnostics.has_errors(), "validator should accept valid values, enums, and events");
}

void validator_rejects_invalid_values_enums_and_events()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          value orderAmount: MissingType;

          enum orderStatus {
            pending = "pending"
            pending = "duplicate"
          }

          enum EmptyStatus {
          }

          event orderAccepted {
            fields {
              order_id MissingType
              order_id string
            }
          }

          event EmptyEvent {
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4501"), "validator should reject invalid value names"
    );
    require(has_error_code(diagnostics, "SSPEC4601"), "validator should reject invalid enum names");
    require(
        has_error_code(diagnostics, "SSPEC4602"), "validator should reject invalid enum members"
    );
    require(
        has_error_code(diagnostics, "SSPEC4701"), "validator should reject invalid event names"
    );
    require(
        has_error_code(diagnostics, "SSPEC3001"),
        "validator should reject duplicate enum members and event fields"
    );
    require(
        has_error_code(diagnostics, "SSPEC3002"),
        "validator should reject unknown value and event field types"
    );
    require(
        has_error_code(diagnostics, "SSPEC4001"), "validator should reject empty enums and events"
    );
}

void validator_accepts_external_systems_and_policies()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartInvoiceRequest {
            tenant_id string
            invoice_id string
          }
          shape StartInvoiceResponse {
            accepted bool
          }

          entity ExternalSystemEndpoint {
            key tenant_id, external_system_id, profile
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
              external_system_id string
              profile string
              base_url string
              auth_ref string
              timeout_ms int
            }
            indexes {
              unique by_tenant_system_profile on tenant_id, external_system_id, profile
            }
            state_machine {
              state Active
              state Disabled
              state Deleted {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Active
              terminal Deleted
              Active -> Disabled
              Disabled -> Active
              Active -> Deleted
              Disabled -> Deleted
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
                metadata.timeout_ms -> client.timeout_ms
                input.invoice_id -> request.invoice_id
              }
            }
          }

          api StartInvoice {
            method POST
            path "/v1/tenants/{tenantId}/invoices/start"
            input StartInvoiceRequest
            output StartInvoiceResponse
          }

          policy BillingAccess {
            tenant scoped_by tenant_id
            allow StartInvoice when caller.role == billing_admin;
            quota starts_per_minute: 60;
            audit StartInvoice;
          }
        }
    )sspec");

    require(
        !diagnostics.has_errors(), "validator should accept valid external systems and policies"
    );
}

void validator_rejects_invalid_external_systems()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          external_system stripe {
            owner: "payments"
            owner: "duplicate"
            metadata {
              entity ExternalSystemEndpoint
              profile_field profile
              required_fields [base_url, base_url]
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4901"),
        "validator should reject invalid external system names"
    );
    require(
        has_error_code(diagnostics, "SSPEC3001"),
        "validator should reject duplicate external system properties"
    );
    require(
        has_error_code(diagnostics, "SSPEC3001"),
        "validator should reject duplicate external system required metadata fields"
    );
}

void validator_rejects_invalid_external_system_metadata_model()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          entity ExternalSystemEndpoint {
            key external_system_id, profile
            ownership {
              authority: external
              system_of_record: self
              lifecycle: observed
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              external_system_id string
              profile string
              base_url string
            }
            state_machine {
              state Active
              state Deleted {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Active
              terminal Deleted
              Active -> Deleted
            }
          }

          external_system Stripe {
            metadata {
              entity ExternalSystemEndpoint
              profile_field missing_profile
              required_fields [base_url, auth_ref]
            }
          }

          external_system MissingMetadataTarget {
            metadata {
              entity MissingEndpoint
              profile_field profile
              required_fields [base_url]
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC3002"),
        "validator should reject unknown external system metadata entities"
    );
    require(
        has_error_code(diagnostics, "SSPEC4902"),
        "validator should reject metadata profile fields missing from the metadata entity"
    );
    require(
        has_error_code(diagnostics, "SSPEC4903"),
        "validator should reject metadata required fields missing from the metadata entity"
    );
    require(
        has_error_code(diagnostics, "SSPEC4904"),
        "validator should reject metadata entities missing the tenant field"
    );
    require(
        has_error_code(diagnostics, "SSPEC4905"),
        "validator should reject metadata entity keys missing the tenant field"
    );
    require(
        has_error_code(diagnostics, "SSPEC4911"),
        "validator should require authoritative system ownership for metadata entities"
    );
    require(
        has_error_code(diagnostics, "SSPEC4913"),
        "validator should require metadata profile fields to be part of the entity key"
    );
    require(
        has_error_code(diagnostics, "SSPEC4914"),
        "validator should require a unique metadata entity index on key fields"
    );
    require(
        has_error_code(diagnostics, "SSPEC4915"),
        "validator should require canonical metadata entity lifecycle states"
    );
    require(
        has_error_code(diagnostics, "SSPEC4917"),
        "validator should require metadata entity lifecycle transitions"
    );
}

void validator_rejects_invalid_external_system_metadata_mappings()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          entity ExternalSystemEndpoint {
            key tenant_id, external_system_id, profile
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
              external_system_id string
              profile string
              base_url string
              auth_ref string
            }
            state_machine {
              state Active
              state Deleted {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Active
              terminal Deleted
              Active -> Deleted
            }
          }

          external_system Stripe {
            metadata {
              entity ExternalSystemEndpoint
              profile_field profile
              required_fields [base_url]
              mappings {
                metadata.auth_ref -> client.auth_ref
                metadata.missing -> client.missing
                operator.base_url -> client.operator_base_url
                metadata.base_url -> response.base_url
                input -> request.id
                input.order_id -> request.id
                entity.order_id -> request.id
              }
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, "SSPEC4906"),
        "validator should reject metadata mapping paths without a field segment"
    );
    require(
        has_error_code(diagnostics, "SSPEC4907"),
        "validator should reject unsupported metadata mapping roots"
    );
    require(
        has_error_code(diagnostics, "SSPEC4908"),
        "validator should reject duplicate metadata mapping targets"
    );
    require(
        has_error_code(diagnostics, "SSPEC4909"),
        "validator should reject metadata mapping fields missing from the metadata entity"
    );
    require(
        has_error_code(diagnostics, "SSPEC4910"),
        "validator should require mapped metadata fields to be required metadata fields"
    );
}

} // namespace

TEST_CASE("validator rejects invalid shapes")
{
    validator_rejects_invalid_shapes();
}

TEST_CASE("validator accepts values, enums, and events")
{
    validator_accepts_values_enums_and_events();
}

TEST_CASE("validator rejects invalid values, enums, and events")
{
    validator_rejects_invalid_values_enums_and_events();
}

TEST_CASE("validator accepts external systems and policies")
{
    validator_accepts_external_systems_and_policies();
}

TEST_CASE("validator rejects invalid external systems")
{
    validator_rejects_invalid_external_systems();
}

TEST_CASE("validator rejects invalid external system metadata model")
{
    validator_rejects_invalid_external_system_metadata_model();
}

TEST_CASE("validator rejects invalid external system metadata mappings")
{
    validator_rejects_invalid_external_system_metadata_mappings();
}
