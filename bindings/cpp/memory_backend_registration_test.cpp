#include "memory/backend.hpp"

#include "catch2/catch_amalgamated.hpp"

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
    }
}
