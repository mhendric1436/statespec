package com.statespec.backend.runtime;

import com.statespec.backend.Backend;
import com.statespec.backend.Json;
import com.statespec.backend.Log;
import com.statespec.backend.Metric;
import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

final class ObservabilityCodec
{
    private ObservabilityCodec() {}

    static Json fieldDescriptorToJson(Backend.FieldDescriptor field)
    {
        return Json.object(Map.of(
            "name", Json.string(field.name()), "type", Json.string(field.type().name()),
            "type_name", Json.string(field.typeName()), "required", Json.bool(field.required())
        ));
    }

    static Backend.FieldDescriptor fieldDescriptorFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Backend.FieldDescriptor(
            Codec.stringFromJson(object.get("name")),
            Backend.FieldType.valueOf(Codec.stringFromJson(object.get("type"))),
            Codec.stringFromJson(object.get("type_name")),
            Codec.boolFromJson(object.get("required"))
        );
    }

    static Json fieldsToJson(List<Backend.FieldDescriptor> fields)
    {
        var values = new ArrayList<Json>();
        for (var field : fields)
        {
            values.add(fieldDescriptorToJson(field));
        }
        return Json.array(values);
    }

    static List<Backend.FieldDescriptor> fieldsFromJson(Json value)
    {
        var fields = new ArrayList<Backend.FieldDescriptor>();
        for (var field : ((Json.ArrayValue)value).values())
        {
            fields.add(fieldDescriptorFromJson(field));
        }
        return List.copyOf(fields);
    }

    static Json logDefinitionToJson(Log.Definition definition)
    {
        return Json.object(Map.of(
            "name", Json.string(definition.name()), "level", Json.string(definition.level().name()),
            "event_name", Json.string(definition.eventName()), "fields",
            fieldsToJson(definition.fields())
        ));
    }

    static Log.Definition logDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Log.Definition(
            Codec.stringFromJson(object.get("name")),
            Log.Level.valueOf(Codec.stringFromJson(object.get("level"))),
            Codec.stringFromJson(object.get("event_name")), fieldsFromJson(object.get("fields"))
        );
    }

    static Json logEventToJson(Log.Event event)
    {
        return Json.object(Map.of(
            "name", Json.string(event.name()), "level", Json.string(event.level().name()),
            "event_name", Json.string(event.eventName()), "fields", Json.object(event.fields())
        ));
    }

    static Log.Event logEventFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Log.Event(
            Codec.stringFromJson(object.get("name")),
            Log.Level.valueOf(Codec.stringFromJson(object.get("level"))),
            Codec.stringFromJson(object.get("event_name")),
            ((Json.ObjectValue)object.get("fields")).values()
        );
    }

    static Json metricDefinitionToJson(Metric.Definition definition)
    {
        return Json.object(Map.of(
            "name", Json.string(definition.name()), "kind", Json.string(definition.kind().name()),
            "backend_name", Json.string(definition.backendName()), "unit",
            Json.string(definition.unit()), "labels", fieldsToJson(definition.labels())
        ));
    }

    static Metric.Definition metricDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Metric.Definition(
            Codec.stringFromJson(object.get("name")),
            Metric.Kind.valueOf(Codec.stringFromJson(object.get("kind"))),
            Codec.stringFromJson(object.get("backend_name")),
            Codec.stringFromJson(object.get("unit")), fieldsFromJson(object.get("labels"))
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
