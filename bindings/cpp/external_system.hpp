#pragma once

#include "backend.hpp"

#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace statespec::backend
{

struct ExternalSystemMetadataKeyValue
{
    std::string field;
    Json value;
};

struct ExternalSystemMetadataLookup
{
    std::string external_system;
    std::string metadata_entity;
    std::optional<std::string> tenant_field;
    std::string profile_field;
    std::vector<std::string> key_fields;
    std::vector<ExternalSystemMetadataKeyValue> key_values;
    std::vector<std::string> required_fields;

    bool key_complete() const;
};

struct ExternalSystemMetadataResolution
{
    VersionedRecord record;
    std::vector<std::string> missing_required_fields;

    bool complete() const noexcept
    {
        return missing_required_fields.empty();
    }
};

inline std::vector<std::string> missing_required_metadata_fields(
    const Json& document,
    const std::vector<std::string>& required_fields
)
{
    std::vector<std::string> missing;
    for (const auto& field : required_fields)
    {
        if (!document.contains(field))
        {
            missing.push_back(field);
        }
    }
    return missing;
}

inline std::vector<std::string>
missing_metadata_key_fields(const ExternalSystemMetadataLookup& lookup)
{
    std::unordered_set<std::string> provided;
    for (const auto& key_value : lookup.key_values)
    {
        provided.insert(key_value.field);
    }

    std::vector<std::string> missing;
    for (const auto& key_field : lookup.key_fields)
    {
        if (provided.find(key_field) == provided.end())
        {
            missing.push_back(key_field);
        }
    }
    return missing;
}

inline bool ExternalSystemMetadataLookup::key_complete() const
{
    return missing_metadata_key_fields(*this).empty();
}

class IExternalSystemMetadataResolver
{
  public:
    virtual ~IExternalSystemMetadataResolver() = default;

    virtual std::optional<ExternalSystemMetadataResolution> resolve_metadata(
        IBackend& backend,
        const ExternalSystemMetadataLookup& lookup
    ) = 0;

    virtual std::optional<ExternalSystemMetadataResolution> resolve_metadataTx(
        ITransaction& tx,
        const ExternalSystemMetadataLookup& lookup
    ) = 0;
};

} // namespace statespec::backend
