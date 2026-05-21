#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"

namespace
{

void parser_parses_empty_system()
{
    const auto spec = statespec::test::parse_text("statespec 0.1; system OrderSystem {}");
    statespec::test::require(spec.version == "0.1", "parser should capture version");
    statespec::test::require(spec.system.has_value(), "parser should capture system");
    statespec::test::require(
        spec.system->name == "OrderSystem", "parser should capture system name"
    );
}

void parser_parses_include_declarations()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        statespec 0.1;
        include "./workflow-launch-control.sspec";
        include "../shared/tenant-model.sspec";

        system OrderSystem {}
    )sspec");

    statespec::test::require(spec.includes.size() == 2, "parser should parse includes");
    statespec::test::require(
        spec.includes[0].path == "./workflow-launch-control.sspec",
        "parser should strip quotes from include path"
    );
    statespec::test::require(
        spec.includes[1].path == "../shared/tenant-model.sspec",
        "parser should preserve relative include path"
    );
    statespec::test::require(spec.system.has_value(), "parser should still parse root system");
}

void parser_rejects_import_declarations()
{
    statespec::DiagnosticBag diagnostics;
    auto spec = statespec::test::parse_text(
        R"sspec(
        statespec 0.1;
        import Shared.Model;

        system OrderSystem {}
    )sspec",
        diagnostics
    );

    statespec::test::require(
        !spec.system.has_value(), "parser should not accept import before the root system"
    );
    statespec::test::require(
        diagnostics.has_errors(), "parser should report import as unsupported syntax"
    );
}

void parser_tracks_system_member_order()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured
          shape StartOrderRequest {
            tenant_id string
          }
          feature_flag NewScheduler {
            type bool
            default false
            scope tenant
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(
        spec.system->member_order.size() == 4, "parser should track system member order"
    );
    statespec::test::require(
        spec.system->member_order[0].kind == "tenant", "parser should track tenant member"
    );
    statespec::test::require(
        spec.system->member_order[2].kind == "shape", "parser should track shape member"
    );
}

void parser_rejects_namespace_blocks()
{
    statespec::DiagnosticBag diagnostics;
    (void)statespec::test::parse_text(
        R"sspec(
        system OrderSystem {
          namespace Billing {
            value InvoiceId: uuid
          }
        }
    )sspec",
        diagnostics
    );

    statespec::test::require(diagnostics.has_errors(), "parser should reject namespace blocks");
}

void parser_reports_missing_system()
{
    statespec::DiagnosticBag diagnostics;
    auto spec = statespec::test::parse_text("entity Order {}", diagnostics);
    statespec::test::require(
        !spec.system.has_value(), "parser should not synthesize missing system"
    );
    statespec::test::require(diagnostics.has_errors(), "parser should report missing system");
}
} // namespace

TEST_CASE("parser parses empty systems")
{
    parser_parses_empty_system();
}

TEST_CASE("parser parses include declarations")
{
    parser_parses_include_declarations();
}

TEST_CASE("parser rejects import declarations")
{
    parser_rejects_import_declarations();
}

TEST_CASE("parser tracks system member order")
{
    parser_tracks_system_member_order();
}

TEST_CASE("parser rejects namespace blocks")
{
    parser_rejects_namespace_blocks();
}

TEST_CASE("parser reports missing systems")
{
    parser_reports_missing_system();
}
