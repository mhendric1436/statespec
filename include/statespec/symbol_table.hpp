#pragma once

#include "statespec/source.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace statespec
{

enum class SymbolKind
{
    System,
    Namespace,
    Value,
    Enum,
    Shape,
    Entity,
    Event,
    Queue,
    Message,
    Lease,
    Worker,
    Api,
    Workflow,
    WorkflowStep,
    Policy,
    GenerateTarget,
};

struct Symbol
{
    SymbolKind kind = SymbolKind::System;
    std::string name;
    SourceRange range;
};

class SymbolTable
{
  public:
    bool insert(Symbol symbol);
    std::optional<Symbol> find(const std::string& name) const;

  private:
    std::unordered_map<std::string, Symbol> symbols_;
};

} // namespace statespec
