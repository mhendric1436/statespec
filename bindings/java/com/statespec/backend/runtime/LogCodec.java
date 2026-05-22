package com.statespec.backend.runtime;

import com.statespec.backend.Json;
import com.statespec.backend.Log;
import java.util.Map;

final class LogCodec
{
    private LogCodec() {}

    static Json logDefinitionToJson(Log.Definition definition)
    {
        return Json.object(Map.of(
            "name", Json.string(definition.name()), "level", Json.string(definition.level().name()),
            "event_name", Json.string(definition.eventName()), "fields",
            ObservabilityCodec.fieldsToJson(definition.fields())
        ));
    }

    static Log.Definition logDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Log.Definition(
            Codec.stringFromJson(object.get("name")),
            Log.Level.valueOf(Codec.stringFromJson(object.get("level"))),
            Codec.stringFromJson(object.get("event_name")),
            ObservabilityCodec.fieldsFromJson(object.get("fields"))
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
}
