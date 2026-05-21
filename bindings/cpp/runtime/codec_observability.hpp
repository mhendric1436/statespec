#pragma once

#include "codec_core.hpp"

#include "../log.hpp"
#include "../metric.hpp"

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

inline std::string metric_kind_name(MetricKind kind)
{
    switch (kind)
    {
    case MetricKind::Counter:
        return "counter";
    case MetricKind::Gauge:
        return "gauge";
    case MetricKind::Histogram:
        return "histogram";
    }
    throw BackendError("unknown metric kind");
}

inline MetricKind metric_kind_from_string(const std::string& value)
{
    if (value == "counter")
    {
        return MetricKind::Counter;
    }
    if (value == "gauge")
    {
        return MetricKind::Gauge;
    }
    if (value == "histogram")
    {
        return MetricKind::Histogram;
    }
    throw BackendError("unknown metric kind: " + value);
}

inline Json metric_definition_to_json(const MetricDefinition& definition)
{
    return Json::object(
        {{"name", definition.name},
         {"kind", metric_kind_name(definition.kind)},
         {"backend_name", definition.backend_name},
         {"unit", definition.unit}}
    );
}

inline MetricDefinition metric_definition_from_json(const Json& json)
{
    return MetricDefinition{
        .name = json.at("name").as_string(),
        .kind = metric_kind_from_string(json.at("kind").as_string()),
        .backend_name = json.at("backend_name").as_string(),
        .unit = json.at("unit").as_string(),
    };
}

inline Json metric_sample_to_json(const MetricSample& sample)
{
    return Json::object(
        {{"name", sample.name},
         {"kind", metric_kind_name(sample.kind)},
         {"backend_name", sample.backend_name},
         {"value", sample.value},
         {"unit", sample.unit},
         {"labels", sample.labels}}
    );
}

inline MetricSample metric_sample_from_json(const Json& json)
{
    return MetricSample{
        .name = json.at("name").as_string(),
        .kind = metric_kind_from_string(json.at("kind").as_string()),
        .backend_name = json.at("backend_name").as_string(),
        .value = json.at("value").as_double(),
        .unit = json.at("unit").as_string(),
        .labels = json.contains("labels") ? json.at("labels") : Json::object({}),
    };
}

} // namespace statespec::backend::runtime::detail
