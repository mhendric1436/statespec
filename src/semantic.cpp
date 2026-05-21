#include "statespec/semantic.hpp"

#include "semantic_declarations.hpp"
#include "semantic_entities.hpp"
#include "semantic_external_systems.hpp"
#include "semantic_observability.hpp"
#include "semantic_policies.hpp"
#include "semantic_runtime.hpp"
#include "semantic_symbols.hpp"
#include "semantic_workflows.hpp"

namespace statespec
{

SemanticSystem resolve_semantics(const Spec& spec)
{
    SemanticSystem resolved;
    if (!spec.system.has_value())
    {
        return resolved;
    }

    const auto& system = *spec.system;
    const auto symbols = build_symbols(system);

    resolve_semantic_declarations(system, resolved);
    resolve_semantic_external_systems(system, resolved);
    resolve_semantic_observability(system, symbols, resolved);
    resolve_semantic_entities(system, symbols, resolved);
    resolve_semantic_runtime(system, symbols, resolved);
    resolve_semantic_workflows(system, symbols, resolved);
    resolve_semantic_policies(system, symbols, resolved);

    return resolved;
}

} // namespace statespec
