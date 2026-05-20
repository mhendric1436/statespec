#pragma once

#include "../backend.hpp"
#include "../feature_flag.hpp"
#include "../lease.hpp"
#include "../log.hpp"
#include "../metric.hpp"
#include "../queue.hpp"
#include "../workflow.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>

namespace statespec::backend::memory
{

namespace detail
{

struct InMemoryState
{
    std::mutex mutex;
    std::map<CollectionName, std::map<Key, VersionedRecord>> records;
    std::map<std::string, Version> versions;
    std::map<CollectionName, CollectionDescriptor> collections;

    std::map<std::string, FeatureFlagDefinition> feature_flag_definitions;
    std::map<std::string, FeatureFlagValue> feature_flag_values;

    std::map<std::string, QueueDefinition> queue_definitions;
    std::map<std::string, QueueMessageRecord> queue_messages;
    std::map<std::string, std::string> queue_idempotency_keys;

    std::map<std::string, LeaseDefinition> lease_definitions;
    std::map<std::string, LeaseRecord> leases;

    std::map<std::string, WorkflowDefinition> workflow_definitions;
    std::map<std::string, WorkflowExecutionRecord> workflow_executions;

    std::map<std::string, LogDefinition> log_definitions;
    std::vector<LogEvent> log_events;

    std::map<std::string, MetricDefinition> metric_definitions;
    std::vector<MetricSample> metric_samples;
};

inline std::string record_version_key(
    const CollectionName& collection,
    const Key& key
)
{
    return "record:" + collection + ":" + key;
}

inline Version version_or_zero(
    const std::map<
        std::string,
        Version>& versions,
    const std::string& key
)
{
    const auto iter = versions.find(key);
    return iter == versions.end() ? 0 : iter->second;
}

} // namespace detail

class InMemoryTransaction : public ITransaction
{
  public:
    explicit InMemoryTransaction(std::shared_ptr<detail::InMemoryState> state)
        : state_(std::move(state))
    {
    }

    bool is_open() const override
    {
        return open_;
    }

    void abort() override
    {
        open_ = false;
        read_versions_.clear();
        puts_.clear();
        erases_.clear();
        feature_flag_definition_puts_.clear();
        feature_flag_value_puts_.clear();
        queue_definition_puts_.clear();
        queue_message_puts_.clear();
        queue_idempotency_puts_.clear();
        lease_definition_puts_.clear();
        lease_puts_.clear();
        lease_erases_.clear();
        workflow_definition_puts_.clear();
        workflow_execution_puts_.clear();
        log_definition_puts_.clear();
        log_event_appends_.clear();
        metric_definition_puts_.clear();
        metric_sample_appends_.clear();
    }

    detail::InMemoryState& state()
    {
        return *state_;
    }

    void record_read(
        const std::string& version_key,
        Version version
    )
    {
        read_versions_.emplace(version_key, version);
    }

    void stage_put(
        const CollectionName& collection,
        const Key& key,
        Json document
    )
    {
        puts_[detail::record_version_key(collection, key)] =
            VersionedRecord{collection, key, 0, std::move(document)};
        erases_.erase(detail::record_version_key(collection, key));
    }

    void stage_erase(
        const CollectionName& collection,
        const Key& key
    )
    {
        const auto version_key = detail::record_version_key(collection, key);
        puts_.erase(version_key);
        erases_.insert(version_key);
    }

    const std::map<
        std::string,
        Version>&
    read_versions() const
    {
        return read_versions_;
    }

    std::map<
        std::string,
        VersionedRecord>&
    puts()
    {
        return puts_;
    }

    std::set<std::string>& erases()
    {
        return erases_;
    }

    std::map<
        std::string,
        FeatureFlagDefinition>&
    feature_flag_definition_puts()
    {
        return feature_flag_definition_puts_;
    }

    std::map<
        std::string,
        FeatureFlagValue>&
    feature_flag_value_puts()
    {
        return feature_flag_value_puts_;
    }

    std::map<
        std::string,
        QueueDefinition>&
    queue_definition_puts()
    {
        return queue_definition_puts_;
    }

    std::map<
        std::string,
        QueueMessageRecord>&
    queue_message_puts()
    {
        return queue_message_puts_;
    }

    std::map<
        std::string,
        std::string>&
    queue_idempotency_puts()
    {
        return queue_idempotency_puts_;
    }

    std::map<
        std::string,
        LeaseDefinition>&
    lease_definition_puts()
    {
        return lease_definition_puts_;
    }

    std::map<
        std::string,
        LeaseRecord>&
    lease_puts()
    {
        return lease_puts_;
    }

    std::set<std::string>& lease_erases()
    {
        return lease_erases_;
    }

    std::map<
        std::string,
        WorkflowDefinition>&
    workflow_definition_puts()
    {
        return workflow_definition_puts_;
    }

    std::map<
        std::string,
        WorkflowExecutionRecord>&
    workflow_execution_puts()
    {
        return workflow_execution_puts_;
    }

    std::map<
        std::string,
        LogDefinition>&
    log_definition_puts()
    {
        return log_definition_puts_;
    }

    std::vector<LogEvent>& log_event_appends()
    {
        return log_event_appends_;
    }

    std::map<
        std::string,
        MetricDefinition>&
    metric_definition_puts()
    {
        return metric_definition_puts_;
    }

    std::vector<MetricSample>& metric_sample_appends()
    {
        return metric_sample_appends_;
    }

  private:
    std::shared_ptr<detail::InMemoryState> state_;
    bool open_ = true;
    std::map<std::string, Version> read_versions_;
    std::map<std::string, VersionedRecord> puts_;
    std::set<std::string> erases_;

    std::map<std::string, FeatureFlagDefinition> feature_flag_definition_puts_;
    std::map<std::string, FeatureFlagValue> feature_flag_value_puts_;
    std::map<std::string, QueueDefinition> queue_definition_puts_;
    std::map<std::string, QueueMessageRecord> queue_message_puts_;
    std::map<std::string, std::string> queue_idempotency_puts_;
    std::map<std::string, LeaseDefinition> lease_definition_puts_;
    std::map<std::string, LeaseRecord> lease_puts_;
    std::set<std::string> lease_erases_;
    std::map<std::string, WorkflowDefinition> workflow_definition_puts_;
    std::map<std::string, WorkflowExecutionRecord> workflow_execution_puts_;
    std::map<std::string, LogDefinition> log_definition_puts_;
    std::vector<LogEvent> log_event_appends_;
    std::map<std::string, MetricDefinition> metric_definition_puts_;
    std::vector<MetricSample> metric_sample_appends_;
};

inline InMemoryTransaction& as_memory_tx(ITransaction& tx)
{
    auto* memory_tx = dynamic_cast<InMemoryTransaction*>(&tx);
    if (memory_tx == nullptr || !memory_tx->is_open())
    {
        throw BackendError("in-memory backend requires an open InMemoryTransaction");
    }
    return *memory_tx;
}

} // namespace statespec::backend::memory
