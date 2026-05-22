package com.statespec.backend.runtime;

import com.statespec.backend.Backend;
import com.statespec.backend.Json;
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
}
