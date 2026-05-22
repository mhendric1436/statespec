#include "schema_compatibility.hpp"

#include "catch2/catch_amalgamated.hpp"

#include <algorithm>

namespace
{

statespec::backend::CollectionDescriptor base_schema_descriptor()
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

void require_schema_reason(
    const statespec::backend::CollectionDescriptor& requested,
    statespec::backend::SchemaCompatibilityReason reason
)
{
    const auto base = base_schema_descriptor();
    const auto result = statespec::backend::compare_collection_descriptors(base, requested);
    REQUIRE(result.status == statespec::backend::SchemaCompatibilityStatus::Incompatible);
    REQUIRE(
        std::any_of(
            result.differences.begin(), result.differences.end(),
            [&](const auto& difference) { return difference.reason == reason; }
        )
    );
}

} // namespace

TEST_CASE("C++ schema compatibility allows identical and compatible descriptor upgrades")
{
    const auto base = base_schema_descriptor();

    const auto identical = statespec::backend::compare_collection_descriptors(base, base);
    REQUIRE(identical.status == statespec::backend::SchemaCompatibilityStatus::Identical);
    REQUIRE(identical.compatible());

    auto optional_field = base;
    optional_field.schema_version = 2;
    optional_field.fields.push_back(
        statespec::backend::FieldDescriptor{
            .name = "description",
            .type = statespec::backend::FieldType::String,
            .type_name = "string",
            .required = false,
        }
    );
    const auto compatible =
        statespec::backend::compare_collection_descriptors(base, optional_field);
    REQUIRE(compatible.status == statespec::backend::SchemaCompatibilityStatus::Compatible);
    REQUIRE(compatible.compatible());

    auto non_unique_index = base;
    non_unique_index.schema_version = 2;
    non_unique_index.indexes.push_back(
        statespec::backend::IndexDescriptor{
            .name = "by_tenant_status",
            .fields = {"tenant_id", "status"},
            .unique = false,
        }
    );
    REQUIRE(
        statespec::backend::compare_collection_descriptors(base, non_unique_index).compatible()
    );
}

TEST_CASE("C++ schema compatibility rejects incompatible descriptor upgrades")
{
    const auto base = base_schema_descriptor();

    auto optional_field = base;
    optional_field.fields.push_back(
        statespec::backend::FieldDescriptor{
            .name = "description",
            .type = statespec::backend::FieldType::String,
            .type_name = "string",
            .required = false,
        }
    );
    require_schema_reason(
        optional_field, statespec::backend::SchemaCompatibilityReason::SchemaVersionNotIncremented
    );

    auto decreased_version = optional_field;
    decreased_version.schema_version = 0;
    require_schema_reason(
        decreased_version, statespec::backend::SchemaCompatibilityReason::SchemaVersionDecreased
    );

    auto changed_key = base;
    changed_key.schema_version = 2;
    changed_key.key_fields = {"tenant_id", "order_id"};
    require_schema_reason(
        changed_key, statespec::backend::SchemaCompatibilityReason::KeyFieldsChanged
    );

    auto removed_field = base;
    removed_field.schema_version = 2;
    removed_field.fields.pop_back();
    require_schema_reason(
        removed_field, statespec::backend::SchemaCompatibilityReason::FieldRemoved
    );

    auto changed_type = base;
    changed_type.schema_version = 2;
    changed_type.fields[1].type = statespec::backend::FieldType::Int;
    require_schema_reason(
        changed_type, statespec::backend::SchemaCompatibilityReason::FieldTypeChanged
    );

    auto changed_type_name = base;
    changed_type_name.schema_version = 2;
    changed_type_name.fields[1].type_name = "Status";
    require_schema_reason(
        changed_type_name, statespec::backend::SchemaCompatibilityReason::FieldTypeNameChanged
    );

    auto became_required = base;
    became_required.schema_version = 2;
    became_required.fields[1].required = true;
    require_schema_reason(
        became_required, statespec::backend::SchemaCompatibilityReason::FieldBecameRequired
    );

    auto required_field = base;
    required_field.schema_version = 2;
    required_field.fields.push_back(
        statespec::backend::FieldDescriptor{
            .name = "priority",
            .type = statespec::backend::FieldType::Int,
            .type_name = "int",
            .required = true,
        }
    );
    require_schema_reason(
        required_field, statespec::backend::SchemaCompatibilityReason::RequiredFieldAdded
    );

    auto removed_index = base;
    removed_index.schema_version = 2;
    removed_index.indexes.clear();
    require_schema_reason(
        removed_index, statespec::backend::SchemaCompatibilityReason::IndexRemoved
    );

    auto changed_index_fields = base;
    changed_index_fields.schema_version = 2;
    changed_index_fields.indexes[0].fields = {"tenant_id", "status"};
    require_schema_reason(
        changed_index_fields, statespec::backend::SchemaCompatibilityReason::IndexFieldsChanged
    );

    auto changed_index_unique = base;
    changed_index_unique.schema_version = 2;
    changed_index_unique.indexes[0].unique = true;
    require_schema_reason(
        changed_index_unique, statespec::backend::SchemaCompatibilityReason::IndexUniquenessChanged
    );

    auto unique_index_added = base;
    unique_index_added.schema_version = 2;
    unique_index_added.indexes.push_back(
        statespec::backend::IndexDescriptor{
            .name = "unique_status",
            .fields = {"status"},
            .unique = true,
        }
    );
    require_schema_reason(
        unique_index_added, statespec::backend::SchemaCompatibilityReason::UniqueIndexAdded
    );
}
