#pragma once

#include "../backend.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>

namespace statespec::backend::memory
{

namespace detail
{

using InMemoryIndexKey = std::vector<std::string>;

struct InMemoryIndexState
{
    IndexDescriptor descriptor;
    std::map<InMemoryIndexKey, std::set<Key>> entries;
};

struct InMemoryState
{
    std::mutex mutex;
    std::map<CollectionName, CollectionDescriptor> collections;
    std::map<CollectionName, std::map<Key, VersionedRecord>> records;
    std::map<CollectionName, std::map<std::string, InMemoryIndexState>> indexes;
    std::map<std::string, Version> versions;
};

inline std::map<
    std::string,
    InMemoryIndexState>
empty_index_states(const CollectionDescriptor& descriptor)
{
    std::map<std::string, InMemoryIndexState> states;
    for (const auto& index : descriptor.indexes)
    {
        states.emplace(index.name, InMemoryIndexState{index, {}});
    }
    return states;
}

inline std::string encode_index_value(const Json* value)
{
    if (value == nullptr)
    {
        return "missing:";
    }
    if (value->is_null())
    {
        return "null:";
    }
    if (value->is_bool())
    {
        return std::string{"bool:"} + (value->as_bool() ? "true" : "false");
    }
    if (value->is_int64())
    {
        return "int:" + value->canonical_string();
    }
    if (value->is_double())
    {
        return "decimal:" + value->canonical_string();
    }
    if (value->is_string())
    {
        return "string:" + value->as_string();
    }
    throw BackendError("in-memory index fields must resolve to scalar JSON values");
}

inline InMemoryIndexKey extract_index_key(
    const Json& document,
    const IndexDescriptor& descriptor
)
{
    InMemoryIndexKey key;
    key.reserve(descriptor.fields.size());
    for (const auto& field : descriptor.fields)
    {
        key.push_back(encode_index_value(document.find(field)));
    }
    return key;
}

inline std::optional<std::pair<
    CollectionName,
    Key>>
parse_record_version_key(const std::string& version_key)
{
    const auto prefix = std::string{"record:"};
    if (version_key.rfind(prefix, 0) != 0)
    {
        return std::nullopt;
    }
    const auto separator = version_key.find(':', prefix.size());
    if (separator == std::string::npos)
    {
        return std::nullopt;
    }
    return std::make_pair(
        version_key.substr(prefix.size(), separator - prefix.size()),
        version_key.substr(separator + 1)
    );
}

inline void remove_record_from_indexes(
    std::map<
        CollectionName,
        std::map<
            std::string,
            InMemoryIndexState>>& indexes,
    const VersionedRecord& record
)
{
    auto collection_iter = indexes.find(record.collection);
    if (collection_iter == indexes.end())
    {
        return;
    }
    for (auto& [_, index] : collection_iter->second)
    {
        const auto key = extract_index_key(record.document, index.descriptor);
        auto entry_iter = index.entries.find(key);
        if (entry_iter == index.entries.end())
        {
            continue;
        }
        entry_iter->second.erase(record.key);
        if (entry_iter->second.empty())
        {
            index.entries.erase(entry_iter);
        }
    }
}

inline void add_record_to_indexes(
    std::map<
        CollectionName,
        std::map<
            std::string,
            InMemoryIndexState>>& indexes,
    const VersionedRecord& record
)
{
    auto collection_iter = indexes.find(record.collection);
    if (collection_iter == indexes.end())
    {
        return;
    }
    for (auto& [_, index] : collection_iter->second)
    {
        const auto key = extract_index_key(record.document, index.descriptor);
        auto& keys = index.entries[key];
        if (index.descriptor.unique)
        {
            for (const auto& existing_key : keys)
            {
                if (existing_key != record.key)
                {
                    throw ConflictError(
                        ConflictKind::UniqueIndexConflict, "unique index conflict on collection '" +
                                                               record.collection + "' index '" +
                                                               index.descriptor.name + "'"
                    );
                }
            }
        }
        keys.insert(record.key);
    }
}

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
    }

    std::optional<VersionedRecord>
    get(const CollectionName& collection,
        const Key& key) override
    {
        require_open();
        const auto version_key = detail::record_version_key(collection, key);

        if (erases_.find(version_key) != erases_.end())
        {
            return std::nullopt;
        }
        if (const auto staged = puts_.find(version_key); staged != puts_.end())
        {
            return staged->second;
        }

        std::lock_guard<std::mutex> lock(state_->mutex);
        const auto collection_iter = state_->records.find(collection);
        if (collection_iter == state_->records.end())
        {
            record_read(version_key, 0);
            return std::nullopt;
        }
        const auto record_iter = collection_iter->second.find(key);
        if (record_iter == collection_iter->second.end())
        {
            record_read(version_key, 0);
            return std::nullopt;
        }
        record_read(version_key, record_iter->second.version);
        return record_iter->second;
    }

    std::vector<VersionedRecord> query(
        const CollectionName& collection,
        const Query& query
    ) override
    {
        require_open();
        std::vector<VersionedRecord> records;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (const auto collection_iter = state_->records.find(collection);
                collection_iter != state_->records.end())
            {
                for (const auto& [_, record] : collection_iter->second)
                {
                    if (puts_.find(detail::record_version_key(record.collection, record.key)) ==
                        puts_.end())
                    {
                        records.push_back(record);
                    }
                }
            }
        }

        for (const auto& [_, record] : puts_)
        {
            if (record.collection == collection)
            {
                records.push_back(record);
            }
        }

        records.erase(
            std::remove_if(
                records.begin(), records.end(),
                [&](const VersionedRecord& record)
                {
                    return erases_.find(
                               detail::record_version_key(record.collection, record.key)
                           ) != erases_.end();
                }
            ),
            records.end()
        );

        std::vector<VersionedRecord> matched;
        for (const auto& record : records)
        {
            if (matches(record, query))
            {
                record_read(
                    detail::record_version_key(record.collection, record.key), record.version
                );
                matched.push_back(record);
            }
        }
        return matched;
    }

    void
    put(const CollectionName& collection,
        const Key& key,
        Json document) override
    {
        require_open();
        (void)get(collection, key);
        const auto version_key = detail::record_version_key(collection, key);
        puts_[version_key] = VersionedRecord{collection, key, 0, std::move(document)};
        erases_.erase(version_key);
    }

    void erase(
        const CollectionName& collection,
        const Key& key
    ) override
    {
        require_open();
        (void)get(collection, key);
        const auto version_key = detail::record_version_key(collection, key);
        puts_.erase(version_key);
        erases_.insert(version_key);
    }

    void commit()
    {
        require_open();
        std::lock_guard<std::mutex> lock(state_->mutex);
        for (const auto& [version_key, expected_version] : read_versions_)
        {
            if (detail::version_or_zero(state_->versions, version_key) != expected_version)
            {
                throw ConflictError(ConflictKind::VersionConflict, "in-memory OCC conflict");
            }
        }

        auto staged_indexes = state_->indexes;
        for (const auto& version_key : erases_)
        {
            const auto parsed = detail::parse_record_version_key(version_key);
            if (!parsed.has_value())
            {
                continue;
            }
            const auto& [collection, key] = *parsed;
            const auto collection_iter = state_->records.find(collection);
            if (collection_iter == state_->records.end())
            {
                continue;
            }
            const auto record_iter = collection_iter->second.find(key);
            if (record_iter != collection_iter->second.end())
            {
                detail::remove_record_from_indexes(staged_indexes, record_iter->second);
            }
        }
        for (const auto& [_, record] : puts_)
        {
            const auto collection_iter = state_->records.find(record.collection);
            if (collection_iter != state_->records.end())
            {
                const auto record_iter = collection_iter->second.find(record.key);
                if (record_iter != collection_iter->second.end())
                {
                    detail::remove_record_from_indexes(staged_indexes, record_iter->second);
                }
            }
            detail::add_record_to_indexes(staged_indexes, record);
        }

        for (const auto& version_key : erases_)
        {
            erase_record(*state_, version_key);
            ++state_->versions[version_key];
        }
        for (auto& [version_key, record] : puts_)
        {
            record.version = ++state_->versions[version_key];
            state_->records[record.collection][record.key] = record;
        }
        state_->indexes = std::move(staged_indexes);

        abort();
    }

  private:
    void require_open() const
    {
        if (!open_)
        {
            throw BackendError("in-memory backend requires an open InMemoryTransaction");
        }
    }

    void record_read(
        const std::string& version_key,
        Version version
    )
    {
        read_versions_.emplace(version_key, version);
    }

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
        const auto parsed = detail::parse_record_version_key(version_key);
        if (!parsed.has_value())
        {
            return;
        }
        const auto& [collection, key] = *parsed;
        if (auto collection_iter = state.records.find(collection);
            collection_iter != state.records.end())
        {
            collection_iter->second.erase(key);
        }
    }

    std::shared_ptr<detail::InMemoryState> state_;
    bool open_ = true;
    std::map<std::string, Version> read_versions_;
    std::map<std::string, VersionedRecord> puts_;
    std::set<std::string> erases_;
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
