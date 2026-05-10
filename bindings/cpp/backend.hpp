#pragma once

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
using Json = std::string;

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

struct Query
{
    enum class Kind
    {
        All,
        KeyPrefix,
        JsonEquals,
    };

    Kind kind = Kind::All;
    std::optional<std::string> key_prefix;
    std::optional<std::string> json_path;
    std::optional<Json> json_value;

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
