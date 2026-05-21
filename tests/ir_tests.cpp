#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/ir.hpp"
#include "statespec/semantic.hpp"

#include <fstream>
#include <sstream>

namespace
{

std::string read_fixture(const std::string& path)
{
    std::ifstream input(path);
    statespec::test::require(input.good(), "fixture should be readable");
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void ir_lowers_kitchen_sink_fixture()
{
    const auto spec =
        statespec::test::parse_text(read_fixture("testdata/parity/kitchen-sink.sspec"));
    const auto resolved = statespec::resolve_semantics(spec);
    const auto ir = statespec::lower_to_ir(resolved);

    statespec::test::require(ir.name == "KitchenSink", "IR should lower fixture system name");
    statespec::test::require(ir.tenant_scope.has_value(), "IR should lower fixture tenant scope");
    statespec::test::require(ir.system_tenant.has_value(), "IR should lower fixture system tenant");
    statespec::test::require(ir.feature_flags.size() == 1, "IR should lower fixture feature flags");
    statespec::test::require(ir.logs.size() == 1, "IR should lower fixture logs");
    statespec::test::require(ir.metrics.size() == 1, "IR should lower fixture metrics");
    statespec::test::require(ir.entities.size() == 2, "IR should lower fixture entities");
    statespec::test::require(ir.queues.size() == 1, "IR should lower fixture queues");
    statespec::test::require(ir.leases.size() == 1, "IR should lower fixture leases");
    statespec::test::require(ir.workers.size() == 1, "IR should lower fixture workers");
    statespec::test::require(ir.api_servers.size() == 1, "IR should lower fixture API servers");
    statespec::test::require(ir.apis.size() == 5, "IR should lower fixture APIs");
    statespec::test::require(ir.workflows.size() == 1, "IR should lower fixture workflows");
    statespec::test::require(ir.policies.size() == 1, "IR should lower fixture policies");

    statespec::test::require(
        resolved.feature_flags[0].scope_target.has_value() &&
            resolved.feature_flags[0].scope_target->kind == statespec::SymbolKind::Entity,
        "semantic resolver should resolve fixture feature flag scope"
    );
    statespec::test::require(
        resolved.workers[0].polls.has_value() &&
            resolved.workers[0].polls->kind == statespec::SymbolKind::Message,
        "semantic resolver should resolve fixture worker poll target"
    );
    statespec::test::require(
        resolved.apis[0].starts_workflow.has_value() &&
            resolved.apis[0].starts_workflow->kind == statespec::SymbolKind::Workflow,
        "semantic resolver should resolve fixture API workflow target"
    );
    statespec::test::require(
        resolved.api_servers[0].serves[0].kind == statespec::SymbolKind::Api,
        "semantic resolver should resolve fixture API server served API"
    );
}
} // namespace

TEST_CASE("IR lowers kitchen sink parity fixture")
{
    ir_lowers_kitchen_sink_fixture();
}
