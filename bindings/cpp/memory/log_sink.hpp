#pragma once

#include "backend.hpp"
#include "codec.hpp"

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
        ensure_collections(backend);
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
        memory_tx.put(
            kDefinitionsCollection, definition.name, detail::log_definition_to_json(definition)
        );
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
        auto record = as_memory_tx(tx).get(kDefinitionsCollection, name);
        if (!record.has_value())
        {
            return std::nullopt;
        }
        return detail::log_definition_from_json(record->document);
    }

    void emit_log(
        IBackend& backend,
        const LogEvent& event
    ) override
    {
        ensure_collections(backend);
        auto tx = backend.begin();
        emit_logTx(*tx, event);
        backend.commit(*tx);
    }

    void emit_logTx(
        ITransaction& tx,
        const LogEvent& event
    ) override
    {
        as_memory_tx(tx).put(
            kEventsCollection, next_event_key(event.name), detail::log_event_to_json(event)
        );
    }

    std::vector<LogEvent> inspect_events(IBackend& backend)
    {
        auto tx = backend.begin();
        auto records = backend.query(*tx, kEventsCollection, Query::all());
        backend.commit(*tx);

        std::vector<LogEvent> events;
        for (const auto& record : records)
        {
            events.push_back(detail::log_event_from_json(record.document));
        }
        return events;
    }

  private:
    static constexpr const char* kDefinitionsCollection = "statespec_log_definitions";
    static constexpr const char* kEventsCollection = "statespec_log_events";

    static void ensure_collections(IBackend& backend)
    {
        backend.ensure_collections(
            {CollectionDescriptor{.name = kDefinitionsCollection, .key_fields = {"name"}},
             CollectionDescriptor{.name = kEventsCollection, .key_fields = {"event_id"}}}
        );
    }

    std::string next_event_key(const std::string& name)
    {
        return name + ":" + std::to_string(++next_event_id_);
    }

    std::uint64_t next_event_id_ = 0;
};

} // namespace statespec::backend::memory
