#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"

namespace
{

void parser_parses_feature_flags()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          feature_flag NewScheduler {
            type bool
            default false
            scope tenant
            owner "platform"
            description "Route order processing through the new scheduler"
            expires "2026-12-31"
          }

          feature_flag MaxPendingOrders {
            type int
            default 100
            scope entity Order
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(
        spec.system->feature_flags.size() == 2, "parser should parse feature flags"
    );

    const auto& bool_flag = spec.system->feature_flags[0];
    statespec::test::require(
        bool_flag.name == "NewScheduler", "parser should parse feature flag name"
    );
    statespec::test::require(bool_flag.type == "bool", "parser should parse feature flag type");
    statespec::test::require(
        bool_flag.default_value == "false", "parser should parse boolean default"
    );
    statespec::test::require(bool_flag.scope == "tenant", "parser should parse tenant scope");
    statespec::test::require(bool_flag.owner == "platform", "parser should parse owner");
    statespec::test::require(
        bool_flag.description == "Route order processing through the new scheduler",
        "parser should parse description"
    );
    statespec::test::require(bool_flag.expires == "2026-12-31", "parser should parse expiration");

    const auto& typed_flag = spec.system->feature_flags[1];
    statespec::test::require(
        typed_flag.name == "MaxPendingOrders", "parser should parse second flag name"
    );
    statespec::test::require(typed_flag.type == "int", "parser should parse typed flag type");
    statespec::test::require(
        typed_flag.default_value == "100", "parser should parse integer default"
    );
    statespec::test::require(
        typed_flag.scope == "entity Order", "parser should parse entity scope"
    );
}

void parser_parses_values_enums_and_events()
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
              status OrderStatus
              amount OrderAmount
            }
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(spec.system->values.size() == 1, "parser should parse values");
    statespec::test::require(
        spec.system->values[0].name == "OrderAmount", "parser should parse value name"
    );
    statespec::test::require(
        spec.system->values[0].type == "decimal", "parser should parse value type"
    );
    statespec::test::require(
        spec.system->values[0].constraint.has_value(), "parser should parse value constraint"
    );

    statespec::test::require(spec.system->enums.size() == 1, "parser should parse enums");
    const auto& enum_decl = spec.system->enums[0];
    statespec::test::require(enum_decl.name == "OrderStatus", "parser should parse enum name");
    statespec::test::require(enum_decl.members.size() == 3, "parser should parse enum members");
    statespec::test::require(
        enum_decl.members[0].value == "pending", "parser should parse enum member value"
    );
    statespec::test::require(
        enum_decl.members[2].name == "Complete", "parser should parse value-less enum member"
    );

    statespec::test::require(spec.system->events.size() == 1, "parser should parse events");
    const auto& event = spec.system->events[0];
    statespec::test::require(event.name == "OrderAccepted", "parser should parse event name");
    statespec::test::require(event.fields.size() == 3, "parser should parse event fields");
    statespec::test::require(
        event.fields[2].type == "OrderAmount", "parser should parse event field value type"
    );
}

void parser_parses_external_systems()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
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

          shape InvoiceRequest {
            invoice_id uuid
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(
        spec.system->external_systems.size() == 1, "parser should parse external systems"
    );
    statespec::test::require(
        spec.system->external_systems[0].name == "Stripe",
        "parser should parse external system name"
    );
    statespec::test::require(
        spec.system->external_systems[0].properties.size() == 2,
        "parser should parse external system properties"
    );
    statespec::test::require(
        spec.system->external_systems[0].metadata.has_value(),
        "parser should parse external system metadata"
    );
    statespec::test::require(
        spec.system->external_systems[0].metadata->entity == "ExternalSystemEndpoint",
        "parser should parse external system metadata entity"
    );
    statespec::test::require(
        spec.system->external_systems[0].metadata->profile_field == "profile",
        "parser should parse external system metadata profile field"
    );
    statespec::test::require(
        spec.system->external_systems[0].metadata->required_fields.size() == 3,
        "parser should parse external system required metadata fields"
    );
    statespec::test::require(
        spec.system->external_systems[0].metadata->mappings.size() == 3,
        "parser should parse external system metadata field mappings"
    );
    statespec::test::require(
        spec.system->external_systems[0].metadata->mappings[0].source == "metadata.base_url",
        "parser should parse external system metadata mapping source"
    );
    statespec::test::require(
        spec.system->external_systems[0].metadata->mappings[0].target == "client.base_url",
        "parser should parse external system metadata mapping target"
    );
    statespec::test::require(
        spec.system->shapes[0].name == "InvoiceRequest", "parser should parse shape names"
    );
}

void parser_parses_shapes()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          shape CreateOrderRequest {
            tenant_id string
            order_id string
            priority int?
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(spec.system->shapes.size() == 1, "parser should parse shapes");
    const auto& shape = spec.system->shapes[0];
    statespec::test::require(shape.name == "CreateOrderRequest", "parser should parse shape name");
    statespec::test::require(shape.fields.size() == 3, "parser should parse shape fields");
    statespec::test::require(
        shape.fields[0].name == "tenant_id", "parser should parse shape field name"
    );
    statespec::test::require(
        shape.fields[2].type == "int?", "parser should parse optional shape field type"
    );
}
} // namespace

TEST_CASE("parser parses feature flags")
{
    parser_parses_feature_flags();
}

TEST_CASE("parser parses values, enums, and events")
{
    parser_parses_values_enums_and_events();
}

TEST_CASE("parser parses external systems")
{
    parser_parses_external_systems();
}

TEST_CASE("parser parses shapes")
{
    parser_parses_shapes();
}
