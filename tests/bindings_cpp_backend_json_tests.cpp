#include "catch2/catch_amalgamated.hpp"
#include "../bindings/cpp/backend.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace
{

class FakeTransaction final : public statespec::backend::ITransaction
{
  public:
    bool is_open() const override
    {
        return open_;
    }

    void abort() override
    {
        open_ = false;
    }

  private:
    bool open_ = true;
};

class FakeBackend final : public statespec::backend::IBackend
{
  public:
    statespec::backend::BackendCapabilities capabilities() const override
    {
        return {};
    }

    void ensure_collection(const statespec::backend::CollectionDescriptor&) override {}

    void ensure_collections(const std::vector<statespec::backend::CollectionDescriptor>&) override {}

    std::unique_ptr<statespec::backend::ITransaction> begin() override
    {
        return std::make_unique<FakeTransaction>();
    }

    std::optional<statespec::backend::VersionedRecord> get(
        statespec::backend::ITransaction&,
        const statespec::backend::CollectionName& collection,
        const statespec::backend::Key& key
    ) override
    {
        return statespec::backend::VersionedRecord{
            .collection = collection,
            .key = key,
            .version = 7,
            .document = last_document,
        };
    }

    std::vector<statespec::backend::VersionedRecord> query(
        statespec::backend::ITransaction&,
        const statespec::backend::CollectionName& collection,
        const statespec::backend::Query& query
    ) override
    {
        last_query = query;
        return {statespec::backend::VersionedRecord{
            .collection = collection,
            .key = "user:1",
            .version = 7,
            .document = last_document,
        }};
    }

    void put(
        statespec::backend::ITransaction&,
        const statespec::backend::CollectionName&,
        const statespec::backend::Key&,
        statespec::backend::Json document
    ) override
    {
        last_document = std::move(document);
    }

    void erase(
        statespec::backend::ITransaction&,
        const statespec::backend::CollectionName&,
        const statespec::backend::Key&
    ) override
    {
    }

    void commit(statespec::backend::ITransaction& tx) override
    {
        tx.abort();
    }

    statespec::backend::Json last_document = statespec::backend::Json::null();
    statespec::backend::Query last_query = statespec::backend::Query::all();
};

} // namespace

TEST_CASE("C++ backend binding carries typed JSON documents")
{
    FakeBackend backend;
    auto tx = backend.begin();

    auto document = statespec::backend::Json::object(
        {{"id", "user:1"}, {"active", true}, {"count", std::int64_t{3}}}
    );
    backend.put(*tx, "users", "user:1", document);

    auto loaded = backend.get(*tx, "users", "user:1");

    REQUIRE(loaded.has_value());
    REQUIRE(loaded->document.is_object());
    REQUIRE(loaded->document["id"].as_string() == "user:1");
    REQUIRE(loaded->document["active"].as_bool());
    REQUIRE(loaded->document["count"].as_int64() == 3);
}

TEST_CASE("C++ backend binding carries typed JSON query values and index values")
{
    FakeBackend backend;
    auto tx = backend.begin();

    auto query = statespec::backend::Query::json_equals("$.active", statespec::backend::Json(true));
    auto rows = backend.query(*tx, "users", query);

    REQUIRE(rows.size() == 1);
    REQUIRE(backend.last_query.json_value.has_value());
    REQUIRE(backend.last_query.json_value->is_bool());
    REQUIRE(backend.last_query.json_value->as_bool());

    auto string_value = statespec::backend::IndexValue::string_value("alice@example.com");
    auto integer_value = statespec::backend::IndexValue::integer_value(42);
    auto decimal_value = statespec::backend::IndexValue::decimal_value(1.5);
    auto boolean_value = statespec::backend::IndexValue::boolean_value(false);
    auto timestamp_value = statespec::backend::IndexValue::timestamp_value("2026-05-12T00:00:00Z");

    REQUIRE(string_value.value.as_string() == "alice@example.com");
    REQUIRE(integer_value.value.as_int64() == 42);
    REQUIRE(decimal_value.value.as_double() == 1.5);
    REQUIRE(!boolean_value.value.as_bool());
    REQUIRE(timestamp_value.value.as_string() == "2026-05-12T00:00:00Z");
}
