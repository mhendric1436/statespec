#pragma once

#include "validator_context.hpp"

#include <string>
#include <vector>

namespace statespec::validator_detail
{

void validate_field_duplicates(
    const std::vector<FieldDecl>& fields,
    DiagnosticBag& diagnostics
);

void validate_field_types(
    const std::vector<FieldDecl>& fields,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

bool is_known_type_reference(
    const std::string& type,
    const SymbolTable& symbols
);

void validate_system_tenancy(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
);

void validate_system_member_order(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
);

void validate_feature_flag_expression(
    const SystemDecl& system,
    const SourceRange& range,
    const std::string& expression,
    DiagnosticBag& diagnostics
);

void validate_feature_flags(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

void validate_namespaces(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

void validate_values(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

void validate_enums(
    DiagnosticBag& diagnostics,
    const SystemDecl& system
);

void validate_events(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

void validate_external_systems(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
);

void validate_shapes(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

void validate_logs(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

void validate_metrics(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

void validate_policies(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

} // namespace statespec::validator_detail
