#pragma once

#include "json.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace statespec::backend
{

using CollectionName = std::string;
using Key = std::string;
using Version = std::uint64_t;

struct FieldDescriptor
{
    std::string name;
    std::string type;
    bool required = false;
};

struct IndexDescriptor
{
    std::string name;
    std::vector<std::string> fields;
    bool unique = false;
};

struct CollectionDescriptor
{
    std::string name;
    std::vector<FieldDescriptor> fields;
    std::vector<std::string> key_fields;
    std::vector<IndexDescriptor> indexes;
    std::uint64_t schema_version = 1;
};

struct BackendCapabilities
{
    bool transactions = true;
    bool compare_and_swap = true;
    bool prefix_query = false;
    bool secondary_indexes = false;
    bool unique_indexes = false;
    bool json_path_query = false;
    bool ordered_scan = false;
    bool durable_history = false;
    bool schema_snapshots = false;
};

enum class ConflictKind
{
    VersionConflict,
    PredicateConflict,
    UniqueIndexConflict,
    SchemaConflict,
    LeaseConflict,
    QueueClaimConflict,
    WorkflowClaimConflict,
};

class BackendError : public std::runtime_error
{
  public:
    explicit BackendError(std::string message)
        : std::runtime_error(std::move(message))
    {
    }
};

class ConflictError : public BackendError
{
  public:
    ConflictError(
        ConflictKind kind,
        std::string message
    )
        : BackendError(std::move(message)), kind_(kind)
    {
    }

    ConflictKind kind() const noexcept
    {
        return kind_;
    }

  private:
    ConflictKind kind_;
};

struct VersionedRecord
{
    CollectionName collection;
    Key key;
    Version version = 0;
    Json document;
};

struct IndexValue
{
    enum class Kind
    {
        Null,
        String,
        Integer,
        Decimal,
        Boolean,
        Timestamp,
    };

    Kind kind = Kind::Null;
    Json value;

    static IndexValue null_value()
    {
        return IndexValue{};
    }

    static IndexValue string_value(std::string value)
    {
        return IndexValue{Kind::String, Json(std::move(value))};
    }

    static IndexValue integer_value(std::int64_t value)
    {
        return IndexValue{Kind::Integer, Json(value)};
    }

    static IndexValue decimal_value(double value)
    {
        return IndexValue{Kind::Decimal, Json(value)};
    }

    static IndexValue boolean_value(bool value)
    {
        return IndexValue{Kind::Boolean, Json(value)};
    }

    static IndexValue timestamp_value(std::string value)
    {
        return IndexValue{Kind::Timestamp, Json(std::move(value))};
    }
};

struct IndexBound
{
    std::vector<IndexValue> values;
    bool inclusive = true;
};

struct Query
{
    enum class Kind
    {
        All,
        KeyPrefix,
        JsonEquals,
        IndexEquals,
        IndexPrefix,
        IndexRange,
    };

    Kind kind = Kind::All;
    std::optional<std::string> key_prefix;
    std::optional<std::string> json_path;
    std::optional<Json> json_value;
    std::optional<std::string> index_name;
    std::vector<IndexValue> index_values;
    std::optional<IndexBound> lower_bound;
    std::optional<IndexBound> upper_bound;

    static Query all()
    {
        return Query{};
    }

    static Query key_prefix_query(std::string prefix)
    {
        Query query;
        query.kind = Kind::KeyPrefix;
        query.key_prefix = std::move(prefix);
        return query;
    }

    static Query json_equals(
        std::string path,
        Json value
    )
    {
        Query query;
        query.kind = Kind::JsonEquals;
        query.json_path = std::move(path);
        query.json_value = std::move(value);
        return query;
    }

    static Query index_equals(
        std::string name,
        std::vector<IndexValue> values
    )
    {
        Query query;
        query.kind = Kind::IndexEquals;
        query.index_name = std::move(name);
        query.index_values = std::move(values);
        return query;
    }

    static Query index_prefix(
        std::string name,
        std::vector<IndexValue> values
    )
    {
        Query query;
        query.kind = Kind::IndexPrefix;
        query.index_name = std::move(name);
        query.index_values = std::move(values);
        return query;
    }

    static Query index_range(
        std::string name,
        std::optional<IndexBound> lower,
        std::optional<IndexBound> upper
    )
    {
        Query query;
        query.kind = Kind::IndexRange;
        query.index_name = std::move(name);
        query.lower_bound = std::move(lower);
        query.upper_bound = std::move(upper);
        return query;
    }
};

class ITransaction
{
  public:
    virtual ~ITransaction() = default;

    virtual bool is_open() const = 0;
    virtual void abort() = 0;
};

class IBackend
{
  public:
    virtual ~IBackend() = default;

    virtual BackendCapabilities capabilities() const = 0;

    virtual void ensure_collection(const CollectionDescriptor& descriptor) = 0;

    virtual void ensure_collections(const std::vector<CollectionDescriptor>& descriptors) = 0;

    virtual std::unique_ptr<ITransaction> begin() = 0;

    virtual std::optional<VersionedRecord> get(
        ITransaction& tx,
        const CollectionName& collection,
        const Key& key
    ) = 0;

    virtual std::vector<VersionedRecord> query(
        ITransaction& tx,
        const CollectionName& collection,
        const Query& query
    ) = 0;

    virtual void put(
        ITransaction& tx,
        const CollectionName& collection,
        const Key& key,
        Json document
    ) = 0;

    virtual void erase(
        ITransaction& tx,
        const CollectionName& collection,
        const Key& key
    ) = 0;

    virtual void commit(ITransaction& tx) = 0;
};

} // namespace statespec::backend
