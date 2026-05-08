#include "statespec/symbol_table.hpp"

#include <utility>

namespace statespec
{

bool SymbolTable::insert(Symbol symbol)
{
    const auto name = symbol.name;
    return symbols_.emplace(name, std::move(symbol)).second;
}

std::optional<Symbol> SymbolTable::find(const std::string& name) const
{
    const auto it = symbols_.find(name);
    if (it == symbols_.end())
    {
        return std::nullopt;
    }
    return it->second;
}

} // namespace statespec
