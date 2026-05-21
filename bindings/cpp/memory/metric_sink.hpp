#pragma once

#include "backend.hpp"
#include "codec.hpp"

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
        ensure_collections(backend);
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
        const auto existing = inspect_definitionTx(tx, definition.name);
        tx.put(
            kDefinitionsCollection, definition.name, detail::metric_definition_to_json(definition)
        );
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
        auto record = tx.get(kDefinitionsCollection, name);
        if (!record.has_value())
        {
            return std::nullopt;
        }
        return detail::metric_definition_from_json(record->document);
    }

    void record_metric(
        IBackend& backend,
        const MetricSample& sample
    ) override
    {
        ensure_collections(backend);
        auto tx = backend.begin();
        record_metricTx(*tx, sample);
        backend.commit(*tx);
    }

    void record_metricTx(
        ITransaction& tx,
        const MetricSample& sample
    ) override
    {
        tx.put(
            kSamplesCollection, next_sample_key(sample.name), detail::metric_sample_to_json(sample)
        );
    }

    std::vector<MetricSample> inspect_samples(IBackend& backend)
    {
        auto tx = backend.begin();
        auto records = backend.query(*tx, kSamplesCollection, Query::all());
        backend.commit(*tx);

        std::vector<MetricSample> samples;
        for (const auto& record : records)
        {
            samples.push_back(detail::metric_sample_from_json(record.document));
        }
        return samples;
    }

  private:
    static constexpr const char* kDefinitionsCollection = "statespec_metric_definitions";
    static constexpr const char* kSamplesCollection = "statespec_metric_samples";

    static void ensure_collections(IBackend& backend)
    {
        backend.ensure_collections(
            {CollectionDescriptor{.name = kDefinitionsCollection, .key_fields = {"name"}},
             CollectionDescriptor{.name = kSamplesCollection, .key_fields = {"sample_id"}}}
        );
    }

    std::string next_sample_key(const std::string& name)
    {
        return name + ":" + std::to_string(++next_sample_id_);
    }

    std::uint64_t next_sample_id_ = 0;
};

} // namespace statespec::backend::memory
