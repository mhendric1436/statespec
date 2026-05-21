#pragma once

#include "codec_core.hpp"

#include "../lease.hpp"

namespace statespec::backend::runtime::detail
{

inline Json lease_definition_id_to_json(const LeaseDefinitionId& id)
{
    return Json::object({{"name", id.name}, {"version", static_cast<std::int64_t>(id.version)}});
}

inline LeaseDefinitionId lease_definition_id_from_json(const Json& json)
{
    return LeaseDefinitionId{
        .name = json.at("name").as_string(),
        .version = static_cast<std::uint64_t>(json.at("version").as_int64()),
    };
}

inline Json lease_definition_to_json(const LeaseDefinition& definition)
{
    Json::Object object{
        {"id", lease_definition_id_to_json(definition.id)},
        {"resource_pattern", definition.resource_pattern},
        {"ttl_seconds", seconds_count(definition.ttl)},
        {"renew_every_seconds", seconds_count(definition.renew_every)},
        {"fencing_token", definition.fencing_token},
    };
    if (definition.max_ttl.has_value())
    {
        object.emplace("max_ttl_seconds", seconds_count(*definition.max_ttl));
    }
    return Json::object(std::move(object));
}

inline LeaseDefinition lease_definition_from_json(const Json& json)
{
    LeaseDefinition definition;
    definition.id = lease_definition_id_from_json(json.at("id"));
    definition.resource_pattern = json.at("resource_pattern").as_string();
    definition.ttl = seconds_from(json.at("ttl_seconds"));
    definition.renew_every = seconds_from(json.at("renew_every_seconds"));
    if (const auto* value = json.find("max_ttl_seconds"))
    {
        definition.max_ttl = seconds_from(*value);
    }
    definition.fencing_token = json.at("fencing_token").as_bool();
    return definition;
}

inline Json lease_record_to_json(const LeaseRecord& lease)
{
    Json::Object object{
        {"definition_id", lease_definition_id_to_json(lease.definition_id)},
        {"resource", lease.resource},
        {"expires_at_ms", timestamp_millis(lease.expires_at)},
        {"fencing_token", static_cast<std::int64_t>(lease.fencing_token)},
    };
    put_optional_string(object, "holder", lease.holder);
    return Json::object(std::move(object));
}

inline LeaseRecord lease_record_from_json(const Json& json)
{
    LeaseRecord record;
    record.definition_id = lease_definition_id_from_json(json.at("definition_id"));
    record.resource = json.at("resource").as_string();
    record.holder = optional_string(json, "holder");
    record.expires_at = timestamp_from(json.at("expires_at_ms"));
    record.fencing_token = static_cast<std::uint64_t>(json.at("fencing_token").as_int64());
    return record;
}

} // namespace statespec::backend::runtime::detail
