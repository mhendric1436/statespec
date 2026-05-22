#pragma once

#include "codec_core.hpp"

#include "../log.hpp"

namespace statespec::backend::runtime::detail
{

inline std::string log_level_name(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug:
        return "debug";
    case LogLevel::Info:
        return "info";
    case LogLevel::Warn:
        return "warn";
    case LogLevel::Error:
        return "error";
    }
    throw BackendError("unknown log level");
}

inline LogLevel log_level_from_string(const std::string& value)
{
    if (value == "debug")
    {
        return LogLevel::Debug;
    }
    if (value == "info")
    {
        return LogLevel::Info;
    }
    if (value == "warn")
    {
        return LogLevel::Warn;
    }
    if (value == "error")
    {
        return LogLevel::Error;
    }
    throw BackendError("unknown log level: " + value);
}

inline Json log_definition_to_json(const LogDefinition& definition)
{
    return Json::object(
        {{"name", definition.name},
         {"level", log_level_name(definition.level)},
         {"event_name", definition.event_name}}
    );
}

inline LogDefinition log_definition_from_json(const Json& json)
{
    return LogDefinition{
        .name = json.at("name").as_string(),
        .level = log_level_from_string(json.at("level").as_string()),
        .event_name = json.at("event_name").as_string(),
    };
}

inline Json log_event_to_json(const LogEvent& event)
{
    return Json::object(
        {{"name", event.name},
         {"level", log_level_name(event.level)},
         {"event_name", event.event_name},
         {"fields", event.fields}}
    );
}

inline LogEvent log_event_from_json(const Json& json)
{
    return LogEvent{
        .name = json.at("name").as_string(),
        .level = log_level_from_string(json.at("level").as_string()),
        .event_name = json.at("event_name").as_string(),
        .fields = json.contains("fields") ? json.at("fields") : Json::object({}),
    };
}

} // namespace statespec::backend::runtime::detail
