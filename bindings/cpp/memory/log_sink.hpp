#pragma once

#include "backend.hpp"

namespace statespec::backend::memory
{

class InMemoryLogSink : public ILogSink
{
  public:
    LogDefinitionRegistration register_definition(
        IBackend& backend,
        const LogDefinition& definition
    ) override
    {
        auto tx = backend.begin();
        auto result = register_definitionTx(*tx, definition);
        backend.commit(*tx);
        return result;
    }

    LogDefinitionRegistration register_definitionTx(
        ITransaction& tx,
        const LogDefinition& definition
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto existing = inspect_definitionTx(memory_tx, definition.name);
        memory_tx.log_definition_puts().insert_or_assign(definition.name, definition);
        return LogDefinitionRegistration{!existing.has_value(), definition};
    }

    std::optional<LogDefinition> inspect_definition(
        IBackend& backend,
        const std::string& name
    ) override
    {
        auto tx = backend.begin();
        auto result = inspect_definitionTx(*tx, name);
        backend.commit(*tx);
        return result;
    }

    std::optional<LogDefinition> inspect_definitionTx(
        ITransaction& tx,
        const std::string& name
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        if (const auto staged = memory_tx.log_definition_puts().find(name);
            staged != memory_tx.log_definition_puts().end())
        {
            return staged->second;
        }
        std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
        if (const auto iter = memory_tx.state().log_definitions.find(name);
            iter != memory_tx.state().log_definitions.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    void emit_log(
        IBackend& backend,
        const LogEvent& event
    ) override
    {
        auto tx = backend.begin();
        emit_logTx(*tx, event);
        backend.commit(*tx);
    }

    void emit_logTx(
        ITransaction& tx,
        const LogEvent& event
    ) override
    {
        as_memory_tx(tx).log_event_appends().push_back(event);
    }

    std::vector<LogEvent> inspect_events(IBackend& backend)
    {
        auto tx = backend.begin();
        auto& memory_tx = as_memory_tx(*tx);
        std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
        return memory_tx.state().log_events;
    }
};

} // namespace statespec::backend::memory
