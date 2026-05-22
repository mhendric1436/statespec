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

inline SchemaCompatibilityResult compare_collection_descriptors(
    const CollectionDescriptor& existing,
    const CollectionDescriptor& requested
)
{
    if (collection_descriptors_equal(existing, requested))
    {
        return SchemaCompatibilityResult{SchemaCompatibilityStatus::Identical, {}};
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
