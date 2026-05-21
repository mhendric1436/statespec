#include "semantic_symbols.hpp"

namespace statespec
{

namespace
{

void insert_symbol(
    SymbolTable& symbols,
    SymbolKind kind,
    const std::string& name,
    const SourceRange& range
)
{
    symbols.insert(Symbol{kind, name, range});
}

} // namespace

std::vector<SemanticField> resolve_fields(const std::vector<FieldDecl>& fields)
{
    std::vector<SemanticField> resolved;
    for (const auto& field : fields)
    {
        resolved.push_back(SemanticField{field.name, field.type});
    }
    return resolved;
}

SymbolTable build_symbols(const SystemDecl& system)
{
    SymbolTable symbols;
    insert_symbol(symbols, SymbolKind::System, system.name, system.range);

    for (const auto& flag : system.feature_flags)
    {
        insert_symbol(symbols, SymbolKind::FeatureFlag, flag.name, flag.range);
    }
    for (const auto& value : system.values)
    {
        insert_symbol(symbols, SymbolKind::Value, value.name, value.range);
    }
    for (const auto& enum_decl : system.enums)
    {
        insert_symbol(symbols, SymbolKind::Enum, enum_decl.name, enum_decl.range);
    }
    for (const auto& event : system.events)
    {
        insert_symbol(symbols, SymbolKind::Event, event.name, event.range);
    }
    for (const auto& shape : system.shapes)
    {
        insert_symbol(symbols, SymbolKind::Shape, shape.name, shape.range);
    }
    for (const auto& external_system : system.external_systems)
    {
        insert_symbol(
            symbols, SymbolKind::ExternalSystem, external_system.name, external_system.range
        );
    }
    for (const auto& log : system.logs)
    {
        insert_symbol(symbols, SymbolKind::Log, log.name, log.range);
    }
    for (const auto& metric : system.metrics)
    {
        insert_symbol(symbols, SymbolKind::Metric, metric.name, metric.range);
    }
    for (const auto& entity : system.entities)
    {
        insert_symbol(symbols, SymbolKind::Entity, entity.name, entity.range);
    }
    for (const auto& queue : system.queues)
    {
        insert_symbol(symbols, SymbolKind::Queue, queue.name, queue.range);
        for (const auto& message : queue.messages)
        {
            insert_symbol(
                symbols, SymbolKind::Message, queue.name + "." + message.name, message.range
            );
        }
    }
    for (const auto& lease : system.leases)
    {
        insert_symbol(symbols, SymbolKind::Lease, lease.name, lease.range);
    }
    for (const auto& worker : system.workers)
    {
        insert_symbol(symbols, SymbolKind::Worker, worker.name, worker.range);
    }
    for (const auto& api_server : system.api_servers)
    {
        insert_symbol(symbols, SymbolKind::ApiServer, api_server.name, api_server.range);
    }
    for (const auto& api : system.apis)
    {
        insert_symbol(symbols, SymbolKind::Api, api.name, api.range);
    }
    for (const auto& workflow : system.workflows)
    {
        insert_symbol(symbols, SymbolKind::Workflow, workflow.name, workflow.range);
        for (const auto& step : workflow.steps)
        {
            insert_symbol(
                symbols, SymbolKind::WorkflowStep, workflow.name + "." + step.name, step.range
            );
        }
    }
    for (const auto& policy : system.policies)
    {
        insert_symbol(symbols, SymbolKind::Policy, policy.name, policy.range);
    }

    return symbols;
}

SemanticReference resolve_reference(
    const SymbolTable& symbols,
    const std::string& name
)
{
    SemanticReference reference;
    reference.name = name;
    if (const auto symbol = symbols.find(name); symbol.has_value())
    {
        reference.kind = symbol->kind;
    }
    return reference;
}

std::optional<SemanticReference> resolve_optional_reference(
    const SymbolTable& symbols,
    const std::optional<std::string>& name
)
{
    if (!name.has_value())
    {
        return std::nullopt;
    }
    return resolve_reference(symbols, *name);
}

} // namespace statespec
