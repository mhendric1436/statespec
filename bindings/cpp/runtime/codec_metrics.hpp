#pragma once

#include "codec_core.hpp"

#include "../metric.hpp"

namespace statespec::backend::runtime::detail
{

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
