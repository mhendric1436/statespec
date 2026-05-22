package com.statespec.backend.runtime;

import com.statespec.backend.Json;
import com.statespec.backend.Metric;
import java.math.BigDecimal;
import java.util.Map;

final class MetricCodec
{
    private MetricCodec() {}

    static Json metricDefinitionToJson(Metric.Definition definition)
    {
        return Json.object(Map.of(
            "name", Json.string(definition.name()), "kind", Json.string(definition.kind().name()),
            "backend_name", Json.string(definition.backendName()), "unit",
            Json.string(definition.unit()), "labels",
            ObservabilityCodec.fieldsToJson(definition.labels())
        ));
    }

    static Metric.Definition metricDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Metric.Definition(
            Codec.stringFromJson(object.get("name")),
            Metric.Kind.valueOf(Codec.stringFromJson(object.get("kind"))),
            Codec.stringFromJson(object.get("backend_name")),
            Codec.stringFromJson(object.get("unit")),
            ObservabilityCodec.fieldsFromJson(object.get("labels"))
        );
    }

    static Json metricSampleToJson(Metric.Sample sample)
    {
        return Json.object(Map.of(
            "name", Json.string(sample.name()), "kind", Json.string(sample.kind().name()),
            "backend_name", Json.string(sample.backendName()), "value",
            Json.decimal(BigDecimal.valueOf(sample.value())), "unit", Json.string(sample.unit()),
            "labels", Json.object(sample.labels())
        ));
    }

    static Metric.Sample metricSampleFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Metric.Sample(
            Codec.stringFromJson(object.get("name")),
            Metric.Kind.valueOf(Codec.stringFromJson(object.get("kind"))),
            Codec.stringFromJson(object.get("backend_name")),
            Codec.doubleFromJson(object.get("value")), Codec.stringFromJson(object.get("unit")),
            ((Json.ObjectValue)object.get("labels")).values()
        );
    }
}
