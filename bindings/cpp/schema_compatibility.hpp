#pragma once

#include "backend.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace statespec::backend
{

enum class SchemaCompatibilityStatus
{
    Identical,
    Compatible,
    Incompatible,
};

enum class SchemaCompatibilityReason
{
    SchemaVersionDecreased,
    SchemaVersionNotIncremented,
    CollectionNameChanged,
    KeyFieldsChanged,
    FieldRemoved,
    FieldRenamed,
    FieldTypeChanged,
    FieldTypeNameChanged,
    RequiredFieldAdded,
    FieldBecameRequired,
    IndexRemoved,
    IndexRenamed,
    IndexFieldsChanged,
    IndexUniquenessChanged,
    UniqueIndexAdded,
};

struct SchemaCompatibilityDifference
{
    SchemaCompatibilityReason reason;
    std::string path;
    std::string message;
};

struct SchemaCompatibilityResult
{
    SchemaCompatibilityStatus status = SchemaCompatibilityStatus::Compatible;
    std::vector<SchemaCompatibilityDifference> differences;

    bool compatible() const
    {
        return status != SchemaCompatibilityStatus::Incompatible;
    }
};

inline bool field_descriptors_equal(
    const FieldDescriptor& left,
    const FieldDescriptor& right
)
{
    return left.name == right.name && left.type == right.type &&
           left.type_name == right.type_name && left.required == right.required;
}

inline bool index_descriptors_equal(
    const IndexDescriptor& left,
    const IndexDescriptor& right
)
{
    return left.name == right.name && left.fields == right.fields && left.unique == right.unique;
}

inline bool collection_descriptors_equal(
    const CollectionDescriptor& left,
    const CollectionDescriptor& right
)
{
    if (left.name != right.name || left.key_fields != right.key_fields ||
        left.schema_version != right.schema_version || left.fields.size() != right.fields.size() ||
        left.indexes.size() != right.indexes.size())
    {
        return false;
    }

    for (std::size_t index = 0; index < left.fields.size(); ++index)
    {
        if (!field_descriptors_equal(left.fields[index], right.fields[index]))
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < left.indexes.size(); ++index)
    {
        if (!index_descriptors_equal(left.indexes[index], right.indexes[index]))
        {
            return false;
        }
    }
    return true;
}

inline const FieldDescriptor* find_field_descriptor(
    const std::vector<FieldDescriptor>& fields,
    const std::string& name
)
{
    for (const auto& field : fields)
    {
        if (field.name == name)
        {
            return &field;
        }
    }
    return nullptr;
}

inline const IndexDescriptor* find_index_descriptor(
    const std::vector<IndexDescriptor>& indexes,
    const std::string& name
)
{
    for (const auto& index : indexes)
    {
        if (index.name == name)
        {
            return &index;
        }
    }
    return nullptr;
}

inline SchemaCompatibilityDifference schema_difference(
    SchemaCompatibilityReason reason,
    std::string path,
    std::string message
)
{
    return SchemaCompatibilityDifference{reason, std::move(path), std::move(message)};
}

inline SchemaCompatibilityResult compare_collection_descriptors(
    const CollectionDescriptor& existing,
    const CollectionDescriptor& requested
)
{
    if (collection_descriptors_equal(existing, requested))
    {
        return SchemaCompatibilityResult{SchemaCompatibilityStatus::Identical, {}};
    }

    std::vector<SchemaCompatibilityDifference> differences;
    auto add = [&](SchemaCompatibilityReason reason, std::string path, std::string message)
    { differences.push_back(schema_difference(reason, std::move(path), std::move(message))); };

    if (existing.name != requested.name)
    {
        add(SchemaCompatibilityReason::CollectionNameChanged, "name",
            "collection name changed from '" + existing.name + "' to '" + requested.name + "'");
    }

    if (requested.schema_version < existing.schema_version)
    {
        add(SchemaCompatibilityReason::SchemaVersionDecreased, "schema_version",
            "schema_version decreased");
    }
    else if (requested.schema_version == existing.schema_version)
    {
        add(SchemaCompatibilityReason::SchemaVersionNotIncremented, "schema_version",
            "schema_version must increase when descriptor shape changes");
    }

    if (existing.key_fields != requested.key_fields)
    {
        add(SchemaCompatibilityReason::KeyFieldsChanged, "key_fields", "key_fields changed");
    }

    for (const auto& existing_field : existing.fields)
    {
        const auto* requested_field = find_field_descriptor(requested.fields, existing_field.name);
        if (requested_field == nullptr)
        {
            add(SchemaCompatibilityReason::FieldRemoved, "fields." + existing_field.name,
                "field '" + existing_field.name + "' was removed");
            continue;
        }
        if (existing_field.type != requested_field->type)
        {
            add(SchemaCompatibilityReason::FieldTypeChanged, "fields." + existing_field.name,
                "field '" + existing_field.name + "' type changed");
        }
        if (existing_field.type_name != requested_field->type_name)
        {
            add(SchemaCompatibilityReason::FieldTypeNameChanged, "fields." + existing_field.name,
                "field '" + existing_field.name + "' type_name changed");
        }
        if (!existing_field.required && requested_field->required)
        {
            add(SchemaCompatibilityReason::FieldBecameRequired, "fields." + existing_field.name,
                "field '" + existing_field.name + "' became required");
        }
    }

    for (const auto& requested_field : requested.fields)
    {
        if (find_field_descriptor(existing.fields, requested_field.name) == nullptr &&
            requested_field.required)
        {
            add(SchemaCompatibilityReason::RequiredFieldAdded, "fields." + requested_field.name,
                "required field '" + requested_field.name + "' was added");
        }
    }

    for (const auto& existing_index : existing.indexes)
    {
        const auto* requested_index = find_index_descriptor(requested.indexes, existing_index.name);
        if (requested_index == nullptr)
        {
            add(SchemaCompatibilityReason::IndexRemoved, "indexes." + existing_index.name,
                "index '" + existing_index.name + "' was removed");
            continue;
        }
        if (existing_index.fields != requested_index->fields)
        {
            add(SchemaCompatibilityReason::IndexFieldsChanged, "indexes." + existing_index.name,
                "index '" + existing_index.name + "' fields changed");
        }
        if (existing_index.unique != requested_index->unique)
        {
            add(SchemaCompatibilityReason::IndexUniquenessChanged, "indexes." + existing_index.name,
                "index '" + existing_index.name + "' uniqueness changed");
        }
    }

    for (const auto& requested_index : requested.indexes)
    {
        if (find_index_descriptor(existing.indexes, requested_index.name) == nullptr &&
            requested_index.unique)
        {
            add(SchemaCompatibilityReason::UniqueIndexAdded, "indexes." + requested_index.name,
                "unique index '" + requested_index.name + "' was added");
        }
    }

    if (!differences.empty())
    {
        return SchemaCompatibilityResult{
            SchemaCompatibilityStatus::Incompatible, std::move(differences)
        };
    }

    return SchemaCompatibilityResult{SchemaCompatibilityStatus::Compatible, {}};
}

inline void validate_collection_descriptor_upgrade(
    const CollectionDescriptor& existing,
    const CollectionDescriptor& requested
)
{
    const auto result = compare_collection_descriptors(existing, requested);
    if (result.compatible())
    {
        return;
    }

    std::string message = "collection schema upgrade is incompatible";
    if (!result.differences.empty())
    {
        message += ": " + result.differences.front().message;
    }
    throw ConflictError(ConflictKind::SchemaConflict, message);
}

} // namespace statespec::backend
