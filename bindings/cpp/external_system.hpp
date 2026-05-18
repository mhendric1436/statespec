#pragma once

#include "backend.hpp"

#include <optional>
#include <string>
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
