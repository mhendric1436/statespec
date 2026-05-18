#include "statespec/validator.hpp"

#include "validator_context.hpp"
#include "validator_declarations.hpp"
#include "validator_entities.hpp"
#include "validator_runtime.hpp"
#include "validator_workflows.hpp"

namespace statespec
{

using namespace validator_detail;

void Validator::validate(
    const Spec& spec,
    DiagnosticBag& diagnostics
)
{
    if (!spec.system.has_value())
    {
        diagnostics.error(SourceRange{}, "SSPEC1001", "spec must contain a system declaration");
        return;
    }

    const auto& system = *spec.system;
    auto symbols = build_symbol_table(system, diagnostics);
    const ValidatorContext context{spec, system, symbols, diagnostics};

    validate_feature_flags(context.system, context.symbols, context.diagnostics);
    validate_namespaces(context.system, context.symbols, context.diagnostics);
    validate_values(context.system, context.symbols, context.diagnostics);
    validate_enums(context.diagnostics, context.system);
    validate_events(context.system, context.symbols, context.diagnostics);
    validate_external_systems(context.system, context.diagnostics);
    validate_shapes(context.system, context.symbols, context.diagnostics);
    validate_logs(context.system, context.symbols, context.diagnostics);
    validate_metrics(context.system, context.symbols, context.diagnostics);
    validate_entities(context.system, context.symbols, context.diagnostics);
    validate_queues(context.system, context.symbols, context.diagnostics);
    validate_leases(context.system, context.diagnostics);
    validate_workflows(context.system, context.symbols, context.diagnostics);
    validate_workers(context.system, context.symbols, context.diagnostics);
    validate_apis(context.system, context.symbols, context.diagnostics);
    validate_api_servers(context.system, context.symbols, context.diagnostics);
    validate_policies(context.system, context.symbols, context.diagnostics);
}

} // namespace statespec
