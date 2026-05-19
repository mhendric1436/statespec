#include "validator_context.hpp"

#include "validator_helpers.hpp"

#include <algorithm>

namespace statespec::validator_detail
{

const EntityDecl* find_entity(
    const SystemDecl& system,
    const std::string& name
)
{
    for (const auto& entity : system.entities)
    {
        if (entity.name == name)
        {
            return &entity;
        }
    }
    return nullptr;
}

bool symbol_is_one_of(
    const Symbol& symbol,
    const std::vector<SymbolKind>& kinds
)
{
    return std::find(kinds.begin(), kinds.end(), symbol.kind) != kinds.end();
}

void validate_symbol_reference(
    const SymbolTable& symbols,
    const SourceRange& range,
    const std::string& kind,
    const std::string& name,
    const std::vector<SymbolKind>& allowed_kinds,
    DiagnosticBag& diagnostics
)
{
    const auto symbol = symbols.find(name);
    if (!symbol.has_value() || !symbol_is_one_of(*symbol, allowed_kinds))
    {
        diagnostics.error(range, "SSPEC3002", "unknown " + kind + " reference '" + name + "'");
    }
}

std::unordered_set<std::string> entity_state_names(const EntityDecl& entity)
{
    std::unordered_set<std::string> states;
    if (!entity.state_machine.has_value())
    {
        return states;
    }
    for (const auto& state : entity.state_machine->states)
    {
        states.insert(state.name);
    }
    return states;
}

void duplicate_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& name
)
{
    diagnostics.error(range, "SSPEC3001", "duplicate declaration '" + name + "'");
}

void unknown_reference_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& kind,
    const std::string& name
)
{
    diagnostics.error(range, "SSPEC3002", "unknown " + kind + " reference '" + name + "'");
}

void required_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4001", subject + " must declare " + field);
}

void positive_integer_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4002", subject + " " + field + " must be a positive integer");
}

void non_negative_integer_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4003", subject + " " + field + " must be non-negative");
}

void duration_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4004", subject + " " + field + " must be an ISO-8601 duration");
}

void add_symbol(
    SymbolTable& symbols,
    DiagnosticBag& diagnostics,
    SymbolKind kind,
    const std::string& name,
    const SourceRange& range
)
{
    if (!symbols.insert(Symbol{kind, name, range}))
    {
        duplicate_error(diagnostics, range, name);
    }
}

SymbolTable build_symbol_table(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    SymbolTable symbols;
    add_symbol(symbols, diagnostics, SymbolKind::System, system.name, system.range);

    for (const auto& entity : system.entities)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Entity, entity.name, entity.range);
    }
    for (const auto& value : system.values)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Value, value.name, value.range);
    }
    for (const auto& enum_decl : system.enums)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Enum, enum_decl.name, enum_decl.range);
    }
    for (const auto& event : system.events)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Event, event.name, event.range);
    }
    for (const auto& shape : system.shapes)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Shape, shape.name, shape.range);
    }
    for (const auto& external_system : system.external_systems)
    {
        add_symbol(
            symbols, diagnostics, SymbolKind::ExternalSystem, external_system.name,
            external_system.range
        );
    }
    for (const auto& flag : system.feature_flags)
    {
        add_symbol(symbols, diagnostics, SymbolKind::FeatureFlag, flag.name, flag.range);
    }
    for (const auto& log : system.logs)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Log, log.name, log.range);
    }
    for (const auto& metric : system.metrics)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Metric, metric.name, metric.range);
    }
    for (const auto& queue : system.queues)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Queue, queue.name, queue.range);
        for (const auto& message : queue.messages)
        {
            add_symbol(
                symbols, diagnostics, SymbolKind::Message, queue_message_name(queue, message),
                message.range
            );
        }
    }
    for (const auto& lease : system.leases)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Lease, lease.name, lease.range);
    }
    for (const auto& worker : system.workers)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Worker, worker.name, worker.range);
    }
    for (const auto& api_server : system.api_servers)
    {
        add_symbol(symbols, diagnostics, SymbolKind::ApiServer, api_server.name, api_server.range);
    }
    for (const auto& api : system.apis)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Api, api.name, api.range);
    }
    for (const auto& workflow : system.workflows)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Workflow, workflow.name, workflow.range);
        for (const auto& step : workflow.steps)
        {
            add_symbol(
                symbols, diagnostics, SymbolKind::WorkflowStep, workflow_step_name(workflow, step),
                step.range
            );
        }
    }
    for (const auto& policy : system.policies)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Policy, policy.name, policy.range);
    }

    return symbols;
}

} // namespace statespec::validator_detail
