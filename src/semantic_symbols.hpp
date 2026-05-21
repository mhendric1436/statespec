#pragma once

#include "statespec/semantic.hpp"

namespace statespec
{

std::vector<SemanticField> resolve_fields(const std::vector<FieldDecl>& fields);

SymbolTable build_symbols(const SystemDecl& system);

SemanticReference resolve_reference(
    const SymbolTable& symbols,
    const std::string& name
);

std::optional<SemanticReference> resolve_optional_reference(
    const SymbolTable& symbols,
    const std::optional<std::string>& name
);

} // namespace statespec
