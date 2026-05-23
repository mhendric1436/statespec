#include "validator_runtime.hpp"

#include "validator_helpers.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace statespec::validator_detail
{

namespace
{

enum class RuntimeDomain
{
    Occ,
    FeatureFlags,
    Queues,
    Leases,
    Workflows,
    Logs,
    Metrics,
    EntityGc,
};

struct RuntimeDependency
{
    RuntimeDomain from;
    RuntimeDomain to;
    SourceRange range;
    std::string subject;
};

std::string_view runtime_domain_name(RuntimeDomain domain)
{
    switch (domain)
    {
    case RuntimeDomain::Occ:
        return "OCC";
    case RuntimeDomain::FeatureFlags:
        return "feature_flags";
    case RuntimeDomain::Queues:
        return "queues";
    case RuntimeDomain::Leases:
        return "leases";
    case RuntimeDomain::Workflows:
        return "workflows";
    case RuntimeDomain::Logs:
        return "logs";
    case RuntimeDomain::Metrics:
        return "metrics";
    case RuntimeDomain::EntityGc:
        return "entity_gc";
    }
    return "unknown";
}

bool runtime_dependency_allowed(
    RuntimeDomain from,
    RuntimeDomain to
)
{
    if (to == RuntimeDomain::Occ || from == to)
    {
        return true;
    }

    if (from == RuntimeDomain::Workflows)
    {
        return to == RuntimeDomain::FeatureFlags || to == RuntimeDomain::Queues ||
               to == RuntimeDomain::Leases || to == RuntimeDomain::Logs ||
               to == RuntimeDomain::Metrics;
    }

    return false;
}

bool expression_uses_feature_flags(const std::string& expression)
{
    return expression.find("feature_enabled") != std::string::npos ||
           expression.find("feature_value") != std::string::npos;
}

std::optional<RuntimeDomain> runtime_domain_for_symbol(const Symbol& symbol)
{
    switch (symbol.kind)
    {
    case SymbolKind::FeatureFlag:
        return RuntimeDomain::FeatureFlags;
    case SymbolKind::Message:
    case SymbolKind::Queue:
        return RuntimeDomain::Queues;
    case SymbolKind::Lease:
        return RuntimeDomain::Leases;
    case SymbolKind::Workflow:
        return RuntimeDomain::Workflows;
    case SymbolKind::Log:
        return RuntimeDomain::Logs;
    case SymbolKind::Metric:
        return RuntimeDomain::Metrics;
    default:
        return std::nullopt;
    }
}

void add_symbol_dependency(
    std::vector<RuntimeDependency>& dependencies,
    RuntimeDomain from,
    const SymbolTable& symbols,
    const SourceRange& range,
    const std::string& subject,
    const std::string& target
)
{
    const auto symbol = symbols.find(target);
    if (!symbol.has_value())
    {
        return;
    }
    const auto to = runtime_domain_for_symbol(*symbol);
    if (!to.has_value())
    {
        return;
    }
    dependencies.push_back(RuntimeDependency{from, *to, range, subject});
}

std::vector<RuntimeDependency> runtime_domain_dependencies(
    const SystemDecl& system,
    const SymbolTable& symbols
)
{
    std::vector<RuntimeDependency> dependencies;

    for (const auto& queue : system.queues)
    {
        if (queue.dead_letter.has_value())
        {
            add_symbol_dependency(
                dependencies, RuntimeDomain::Queues, symbols, queue.range,
                "queue '" + queue.name + "' dead_letter", *queue.dead_letter
            );
        }
    }

    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            for (const auto& statement : step.statements)
            {
                const auto subject = "workflow step '" + workflow_step_name(workflow, step) + "'";
                if (statement.expression.has_value() &&
                    expression_uses_feature_flags(*statement.expression))
                {
                    dependencies.push_back(
                        RuntimeDependency{
                            RuntimeDomain::Workflows,
                            RuntimeDomain::FeatureFlags,
                            statement.range,
                            subject,
                        }
                    );
                }

                for (const auto& assignment : statement.payload)
                {
                    if (expression_uses_feature_flags(assignment.expression))
                    {
                        dependencies.push_back(
                            RuntimeDependency{
                                RuntimeDomain::Workflows,
                                RuntimeDomain::FeatureFlags,
                                assignment.range,
                                subject,
                            }
                        );
                    }
                }

                if (statement.target.has_value())
                {
                    add_symbol_dependency(
                        dependencies, RuntimeDomain::Workflows, symbols, statement.range, subject,
                        *statement.target
                    );
                }
            }
        }
    }

    for (const auto& entity : system.entities)
    {
        if (!entity.state_machine.has_value())
        {
            continue;
        }
        for (const auto& state : entity.state_machine->states)
        {
            if (state.garbage_collection.has_value())
            {
                dependencies.push_back(
                    RuntimeDependency{
                        RuntimeDomain::EntityGc,
                        RuntimeDomain::Occ,
                        state.garbage_collection->range,
                        "entity '" + entity.name + "' garbage_collection",
                    }
                );
            }
        }
    }

    return dependencies;
}

} // namespace

void validate_runtime_domain_dependencies(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& dependency : runtime_domain_dependencies(system, symbols))
    {
        if (runtime_dependency_allowed(dependency.from, dependency.to))
        {
            continue;
        }

        diagnostics.error(
            dependency.range, diagnostic_codes::RuntimeDomainDependencyNotAllowed,
            dependency.subject + " creates disallowed runtime domain dependency " +
                std::string{runtime_domain_name(dependency.from)} + " -> " +
                std::string{runtime_domain_name(dependency.to)}
        );
    }
}

} // namespace statespec::validator_detail
