#pragma once

#include "statespec/ast.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/symbol_table.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace statespec::validator_detail
{

struct ValidatorContext
{
    const Spec& spec;
    const SystemDecl& system;
    const SymbolTable& symbols;
    DiagnosticBag& diagnostics;
};

const EntityDecl* find_entity(
    const SystemDecl& system,
    const std::string& name
);

bool symbol_is_one_of(
    const Symbol& symbol,
    const std::vector<SymbolKind>& kinds
);

void validate_symbol_reference(
    const SymbolTable& symbols,
    const SourceRange& range,
    const std::string& kind,
    const std::string& name,
    const std::vector<SymbolKind>& allowed_kinds,
    DiagnosticBag& diagnostics
);

std::unordered_set<std::string> entity_state_names(const EntityDecl& entity);

void duplicate_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& name
);

void unknown_reference_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& kind,
    const std::string& name
);

void required_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
);

void positive_integer_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
);

void non_negative_integer_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
);

void duration_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
);

void add_symbol(
    SymbolTable& symbols,
    DiagnosticBag& diagnostics,
    SymbolKind kind,
    const std::string& name,
    const SourceRange& range
);

SymbolTable build_symbol_table(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
);

} // namespace statespec::validator_detail
