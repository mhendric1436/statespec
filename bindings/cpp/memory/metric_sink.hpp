#pragma once

#include "backend.hpp"

namespace statespec::backend::memory
{

class InMemoryMetricSink : public IMetricSink
{
  public:
    MetricDefinitionRegistration register_definition(
        IBackend& backend,
        const MetricDefinition& definition
    ) override
    {
        auto tx = backend.begin();
        auto result = register_definitionTx(*tx, definition);
        backend.commit(*tx);
        return result;
    }

    MetricDefinitionRegistration register_definitionTx(
        ITransaction& tx,
        const MetricDefinition& definition
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto existing = inspect_definitionTx(memory_tx, definition.name);
        memory_tx.metric_definition_puts().insert_or_assign(definition.name, definition);
        return MetricDefinitionRegistration{!existing.has_value(), definition};
    }

    std::optional<MetricDefinition> inspect_definition(
        IBackend& backend,
        const std::string& name
    ) override
    {
        auto tx = backend.begin();
        auto result = inspect_definitionTx(*tx, name);
        backend.commit(*tx);
        return result;
    }

    std::optional<MetricDefinition> inspect_definitionTx(
        ITransaction& tx,
        const std::string& name
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        if (const auto staged = memory_tx.metric_definition_puts().find(name);
            staged != memory_tx.metric_definition_puts().end())
        {
            return staged->second;
        }
        std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
        if (const auto iter = memory_tx.state().metric_definitions.find(name);
            iter != memory_tx.state().metric_definitions.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    void record_metric(
        IBackend& backend,
        const MetricSample& sample
    ) override
    {
        auto tx = backend.begin();
        record_metricTx(*tx, sample);
        backend.commit(*tx);
    }

    void record_metricTx(
        ITransaction& tx,
        const MetricSample& sample
    ) override
    {
        as_memory_tx(tx).metric_sample_appends().push_back(sample);
    }

    std::vector<MetricSample> inspect_samples(IBackend& backend)
    {
        auto tx = backend.begin();
        auto& memory_tx = as_memory_tx(*tx);
        std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
        return memory_tx.state().metric_samples;
    }
};

} // namespace statespec::backend::memory
