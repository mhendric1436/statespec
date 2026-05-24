#include "memory/backend.hpp"

#include "catch2/catch_amalgamated.hpp"

#include <string>

namespace
{

statespec::backend::CollectionDescriptor base_descriptor()
{
    return statespec::backend::CollectionDescriptor{
        .name = "orders",
        .fields =
            {
                statespec::backend::FieldDescriptor{
                    .name = "tenant_id",
                    .type = statespec::backend::FieldType::String,
                    .type_name = "string",
                    .required = true,
                },
                statespec::backend::FieldDescriptor{
                    .name = "status",
                    .type = statespec::backend::FieldType::String,
                    .type_name = "string",
                    .required = false,
                },
            },
        .key_fields = {"tenant_id"},
        .indexes =
            {
                statespec::backend::IndexDescriptor{
                    .name = "by_status",
                    .fields = {"status"},
                    .unique = false,
                },
            },
        .schema_version = 1,
    };
}

statespec::backend::CollectionDescriptor compatible_descriptor()
{
    auto descriptor = base_descriptor();
    descriptor.schema_version = 2;
    descriptor.fields.push_back(
        statespec::backend::FieldDescriptor{
            .name = "description",
            .type = statespec::backend::FieldType::String,
            .type_name = "string",
            .required = false,
        }
    );
    return descriptor;
}

statespec::backend::CollectionDescriptor alternate_compatible_descriptor()
{
    auto descriptor = base_descriptor();
    descriptor.schema_version = 2;
    descriptor.fields.push_back(
        statespec::backend::FieldDescriptor{
            .name = "notes",
            .type = statespec::backend::FieldType::String,
            .type_name = "string",
            .required = false,
        }
    );
    return descriptor;
}

statespec::backend::CollectionDescriptor unique_status_descriptor()
{
    auto descriptor = base_descriptor();
    descriptor.indexes[0].unique = true;
    return descriptor;
}

statespec::backend::Json order_document(std::string status)
{
    return statespec::backend::Json::object({{"status", std::move(status)}});
}

} // namespace

TEST_CASE("C++ in-memory backend enforces compatible collection registration")
{
    statespec::backend::memory::InMemoryBackend backend;

    const auto base = base_descriptor();
    REQUIRE_NOTHROW(backend.ensure_collection(base));
    REQUIRE_NOTHROW(backend.ensure_collection(base));
    REQUIRE_NOTHROW(backend.ensure_collection(compatible_descriptor()));

    auto incompatible = compatible_descriptor();
    incompatible.schema_version = 3;
    incompatible.key_fields = {"tenant_id", "order_id"};

    try
    {
        backend.ensure_collection(incompatible);
        FAIL("expected schema conflict");
    }
    catch (const statespec::backend::ConflictError& error)
    {
        REQUIRE(error.kind() == statespec::backend::ConflictKind::SchemaConflict);
        const std::string message = error.what();
        REQUIRE(message.find("orders") != std::string::npos);
        REQUIRE(message.find("KeyFieldsChanged") != std::string::npos);
    }
}

TEST_CASE("C++ in-memory backend applies collection batches atomically")
{
    statespec::backend::memory::InMemoryBackend backend;

    REQUIRE_NOTHROW(backend.ensure_collection(base_descriptor()));

    auto incompatible = base_descriptor();
    incompatible.schema_version = 2;
    incompatible.key_fields = {"tenant_id", "order_id"};

    REQUIRE_THROWS_AS(
        backend.ensure_collections({compatible_descriptor(), incompatible}),
        statespec::backend::ConflictError
    );

    REQUIRE_NOTHROW(backend.ensure_collection(alternate_compatible_descriptor()));
}

TEST_CASE("C++ in-memory index key extraction encodes scalar values deterministically")
{
    const auto index = statespec::backend::IndexDescriptor{
        .name = "by_status",
        .fields = {"status", "priority", "archived", "missing", "cleared"},
        .unique = false,
    };
    const auto document = statespec::backend::Json::object(
        {{"status", "Open"},
         {"priority", std::int64_t{3}},
         {"archived", false},
         {"cleared", statespec::backend::Json::null()}}
    );

    const auto key = statespec::backend::memory::detail::extract_index_key(document, index);

    REQUIRE(
        key == statespec::backend::memory::detail::InMemoryIndexKey{
                   "string:Open", "int:3", "bool:false", "missing:", "null:"
               }
    );

    const auto invalid =
        statespec::backend::Json::object({{"status", statespec::backend::Json::array({"Open"})}});
    REQUIRE_THROWS_AS(
        statespec::backend::memory::detail::extract_index_key(invalid, index),
        statespec::backend::BackendError
    );
}

TEST_CASE("C++ in-memory backend enforces unique indexes on commit")
{
    statespec::backend::memory::InMemoryBackend backend;
    backend.ensure_collection(unique_status_descriptor());

    auto tx1 = backend.begin();
    backend.put(*tx1, "orders", "order-1", order_document("Open"));
    backend.commit(*tx1);

    auto duplicate_tx = backend.begin();
    backend.put(*duplicate_tx, "orders", "order-2", order_document("Open"));
    REQUIRE_THROWS_AS(backend.commit(*duplicate_tx), statespec::backend::ConflictError);

    auto update_tx = backend.begin();
    backend.put(*update_tx, "orders", "order-1", order_document("Closed"));
    backend.commit(*update_tx);

    auto insert_tx = backend.begin();
    backend.put(*insert_tx, "orders", "order-2", order_document("Open"));
    REQUIRE_NOTHROW(backend.commit(*insert_tx));
}
