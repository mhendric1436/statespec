#pragma once

#include "statespec/symbol_table.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

struct SemanticReference
{
    std::string name;
    std::optional<SymbolKind> kind;

    bool resolved() const
    {
        return kind.has_value();
    }
};

struct SemanticField
{
    std::string name;
    std::string type;
};

struct SemanticTenantScope
{
    std::string field_name;
};

struct SemanticSystemTenant
{
    std::string source;
    std::string config_key;
};

} // namespace statespec
