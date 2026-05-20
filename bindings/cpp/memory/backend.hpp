#pragma once

#include "transaction.hpp"

#include <algorithm>

namespace statespec::backend::memory
{

class InMemoryBackend : public IBackend
{
  public:
    BackendCapabilities capabilities() const override
    {
        BackendCapabilities capabilities;
        capabilities.prefix_query = true;
        return capabilities;
    }

    void ensure_collection(const CollectionDescriptor& descriptor) override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->collections.insert_or_assign(descriptor.name, descriptor);
    }

    void ensure_collections(const std::vector<CollectionDescriptor>& descriptors) override
    {
        for (const auto& descriptor : descriptors)
        {
            ensure_collection(descriptor);
        }
    }

    std::unique_ptr<ITransaction> begin() override
    {
        return std::make_unique<InMemoryTransaction>(state_);
    }

    std::optional<VersionedRecord>
    get(ITransaction& tx,
        const CollectionName& collection,
        const Key& key) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto version_key = detail::record_version_key(collection, key);

        if (memory_tx.erases().find(version_key) != memory_tx.erases().end())
        {
            return std::nullopt;
        }
        if (const auto staged = memory_tx.puts().find(version_key);
            staged != memory_tx.puts().end())
        {
            return staged->second;
        }

        std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
        const auto collection_iter = memory_tx.state().records.find(collection);
        if (collection_iter == memory_tx.state().records.end())
        {
            memory_tx.record_read(version_key, 0);
            return std::nullopt;
        }
        const auto record_iter = collection_iter->second.find(key);
        if (record_iter == collection_iter->second.end())
        {
            memory_tx.record_read(version_key, 0);
            return std::nullopt;
        }
        memory_tx.record_read(version_key, record_iter->second.version);
        return record_iter->second;
    }

    std::vector<VersionedRecord> query(
        ITransaction& tx,
        const CollectionName& collection,
        const Query& query
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        std::vector<VersionedRecord> records;
        {
            std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
            if (const auto collection_iter = memory_tx.state().records.find(collection);
                collection_iter != memory_tx.state().records.end())
            {
                for (const auto& [_, record] : collection_iter->second)
                {
                    records.push_back(record);
                }
            }
        }

        for (const auto& [version_key, record] : memory_tx.puts())
        {
            if (record.collection == collection)
            {
                records.push_back(record);
            }
            (void)version_key;
        }

        records.erase(
            std::remove_if(
                records.begin(), records.end(),
                [&](const VersionedRecord& record)
                {
                    return memory_tx.erases().find(
                               detail::record_version_key(record.collection, record.key)
                           ) != memory_tx.erases().end();
                }
            ),
            records.end()
        );

        std::vector<VersionedRecord> matched;
        for (const auto& record : records)
        {
            if (matches(record, query))
            {
                memory_tx.record_read(
                    detail::record_version_key(record.collection, record.key), record.version
                );
                matched.push_back(record);
            }
        }
        return matched;
    }

    void
    put(ITransaction& tx,
        const CollectionName& collection,
        const Key& key,
        Json document) override
    {
        auto& memory_tx = as_memory_tx(tx);
        (void)get(memory_tx, collection, key);
        memory_tx.stage_put(collection, key, std::move(document));
    }

    void erase(
        ITransaction& tx,
        const CollectionName& collection,
        const Key& key
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        (void)get(memory_tx, collection, key);
        memory_tx.stage_erase(collection, key);
    }

    void commit(ITransaction& tx) override
    {
        auto& memory_tx = as_memory_tx(tx);
        std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
        for (const auto& [version_key, expected_version] : memory_tx.read_versions())
        {
            if (detail::version_or_zero(memory_tx.state().versions, version_key) !=
                expected_version)
            {
                throw ConflictError(ConflictKind::VersionConflict, "in-memory OCC conflict");
            }
        }

        for (const auto& version_key : memory_tx.erases())
        {
            erase_record(memory_tx.state(), version_key);
            ++memory_tx.state().versions[version_key];
        }
        for (auto& [version_key, record] : memory_tx.puts())
        {
            record.version = ++memory_tx.state().versions[version_key];
            memory_tx.state().records[record.collection][record.key] = record;
        }

        for (const auto& [key, value] : memory_tx.feature_flag_definition_puts())
        {
            memory_tx.state().feature_flag_definitions.insert_or_assign(key, value);
        }
        for (const auto& [key, value] : memory_tx.feature_flag_value_puts())
        {
            memory_tx.state().feature_flag_values.insert_or_assign(key, value);
        }
        for (const auto& [key, value] : memory_tx.queue_definition_puts())
        {
            memory_tx.state().queue_definitions.insert_or_assign(key, value);
        }
        for (const auto& [key, value] : memory_tx.queue_message_puts())
        {
            memory_tx.state().queue_messages.insert_or_assign(key, value);
        }
        for (const auto& [key, value] : memory_tx.queue_idempotency_puts())
        {
            memory_tx.state().queue_idempotency_keys.insert_or_assign(key, value);
        }
        for (const auto& [key, value] : memory_tx.lease_definition_puts())
        {
            memory_tx.state().lease_definitions.insert_or_assign(key, value);
        }
        for (const auto& key : memory_tx.lease_erases())
        {
            memory_tx.state().leases.erase(key);
        }
        for (const auto& [key, value] : memory_tx.lease_puts())
        {
            memory_tx.state().leases.insert_or_assign(key, value);
        }
        for (const auto& [key, value] : memory_tx.workflow_definition_puts())
        {
            memory_tx.state().workflow_definitions.insert_or_assign(key, value);
        }
        for (const auto& [key, value] : memory_tx.workflow_execution_puts())
        {
            memory_tx.state().workflow_executions.insert_or_assign(key, value);
        }
        for (const auto& [key, value] : memory_tx.log_definition_puts())
        {
            memory_tx.state().log_definitions.insert_or_assign(key, value);
        }
        memory_tx.state().log_events.insert(
            memory_tx.state().log_events.end(), memory_tx.log_event_appends().begin(),
            memory_tx.log_event_appends().end()
        );
        for (const auto& [key, value] : memory_tx.metric_definition_puts())
        {
            memory_tx.state().metric_definitions.insert_or_assign(key, value);
        }
        memory_tx.state().metric_samples.insert(
            memory_tx.state().metric_samples.end(), memory_tx.metric_sample_appends().begin(),
            memory_tx.metric_sample_appends().end()
        );

        memory_tx.abort();
    }

  private:
    static bool matches(
        const VersionedRecord& record,
        const Query& query
    )
    {
        switch (query.kind)
        {
        case Query::Kind::All:
            return true;
        case Query::Kind::KeyPrefix:
            return query.key_prefix.has_value() && record.key.rfind(*query.key_prefix, 0) == 0;
        case Query::Kind::JsonEquals:
            return query.json_path.has_value() && query.json_value.has_value() &&
                   json_path_equals(record.document, *query.json_path, *query.json_value);
        default:
            return true;
        }
    }

    static bool json_path_equals(
        const Json& document,
        const std::string& path,
        const Json& expected
    )
    {
        const Json* current = &document;
        std::size_t begin = 0;
        while (begin <= path.size())
        {
            const auto end = path.find('.', begin);
            const auto segment = path.substr(begin, end == std::string::npos ? end : end - begin);
            current = current->find(segment);
            if (current == nullptr)
            {
                return false;
            }
            if (end == std::string::npos)
            {
                break;
            }
            begin = end + 1;
        }
        return *current == expected;
    }

    static void erase_record(
        detail::InMemoryState& state,
        const std::string& version_key
    )
    {
        const auto prefix = std::string{"record:"};
        if (version_key.rfind(prefix, 0) != 0)
        {
            return;
        }
        const auto separator = version_key.find(':', prefix.size());
        if (separator == std::string::npos)
        {
            return;
        }
        const auto collection = version_key.substr(prefix.size(), separator - prefix.size());
        const auto key = version_key.substr(separator + 1);
        if (auto collection_iter = state.records.find(collection);
            collection_iter != state.records.end())
        {
            collection_iter->second.erase(key);
        }
    }

    std::shared_ptr<detail::InMemoryState> state_ = std::make_shared<detail::InMemoryState>();
};

} // namespace statespec::backend::memory
