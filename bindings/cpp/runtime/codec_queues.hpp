#pragma once

#include "codec_core.hpp"

#include "../queue.hpp"

namespace statespec::backend::runtime::detail
{

inline Json queue_definition_to_json(const QueueDefinition& definition)
{
    Json::Object object{
        {"queue", definition.queue},
        {"channel", definition.channel},
        {"visibility_timeout_seconds", seconds_count(definition.visibility_timeout)},
        {"max_attempts", static_cast<std::int64_t>(definition.max_attempts)},
        {"metadata", definition.metadata},
    };
    put_optional_string(object, "dead_letter_queue", definition.dead_letter_queue);
    return Json::object(std::move(object));
}

inline QueueDefinition queue_definition_from_json(const Json& json)
{
    QueueDefinition definition;
    definition.queue = json.at("queue").as_string();
    definition.channel = json.at("channel").as_string();
    definition.visibility_timeout = seconds_from(json.at("visibility_timeout_seconds"));
    definition.max_attempts = static_cast<std::uint32_t>(json.at("max_attempts").as_int64());
    definition.dead_letter_queue = optional_string(json, "dead_letter_queue");
    definition.metadata = json.contains("metadata") ? json.at("metadata") : Json::object({});
    return definition;
}

inline Json queue_message_to_json(const QueueMessageRecord& message)
{
    Json::Object object{
        {"message_id", message.message_id},
        {"queue", message.queue},
        {"channel", message.channel},
        {"status", message.status},
        {"attempts", static_cast<std::int64_t>(message.attempts)},
        {"payload", message.payload},
    };
    put_optional_string(object, "claimed_by", message.claimed_by);
    if (message.claim_expires_at.has_value())
    {
        object.emplace("claim_expires_at_ms", timestamp_millis(*message.claim_expires_at));
    }
    return Json::object(std::move(object));
}

inline QueueMessageRecord queue_message_from_json(const Json& json)
{
    QueueMessageRecord message;
    message.message_id = json.at("message_id").as_string();
    message.queue = json.at("queue").as_string();
    message.channel = json.at("channel").as_string();
    message.status = json.at("status").as_string();
    message.attempts = static_cast<std::uint64_t>(json.at("attempts").as_int64());
    message.claimed_by = optional_string(json, "claimed_by");
    if (const auto* value = json.find("claim_expires_at_ms"))
    {
        message.claim_expires_at = timestamp_from(*value);
    }
    message.payload = json.contains("payload") ? json.at("payload") : Json::object({});
    return message;
}

} // namespace statespec::backend::runtime::detail
