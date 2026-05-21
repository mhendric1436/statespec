package com.statespec.backend.runtime;

import com.statespec.backend.FeatureFlag;
import com.statespec.backend.Json;
import java.util.Map;

final class FeatureFlagCodec
{
    private FeatureFlagCodec() {}

    static Json featureFlagValueToJson(FeatureFlag.Value value)
    {
        Json encodedValue = switch (value)
        {
            case FeatureFlag.Value.BoolValue boolValue -> Json.bool(boolValue.value());
            case FeatureFlag.Value.StringValue stringValue -> Json.string(stringValue.value());
            case FeatureFlag.Value.IntValue intValue -> Json.integer(intValue.value());
            case FeatureFlag.Value.DecimalValue decimalValue -> Json.decimal(decimalValue.value());
        };
        return Json.object(Map.of("type", Json.string(value.type().name()), "value", encodedValue));
    }

    static FeatureFlag.Value featureFlagValueFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        var type = FeatureFlag.Type.valueOf(Codec.stringFromJson(object.get("type")));
        return switch (type)
        {
            case BOOL -> new FeatureFlag.Value.BoolValue(Codec.boolFromJson(object.get("value")));
            case STRING ->
                new FeatureFlag.Value.StringValue(Codec.stringFromJson(object.get("value")));
            case INT -> new FeatureFlag.Value.IntValue(Codec.longFromJson(object.get("value")));
            case DECIMAL ->
                new FeatureFlag.Value.DecimalValue(
                    ((Json.DecimalValue)object.get("value")).value()
                );
        };
    }

    static Json featureFlagDefinitionToJson(FeatureFlag.Definition definition)
    {
        return Json.object(Map.of(
            "name", Json.string(definition.name()), "type", Json.string(definition.type().name()),
            "default_value", featureFlagValueToJson(definition.defaultValue()), "scope",
            Json.string(definition.scope().name()), "owner",
            Codec.optionalString(definition.owner()), "description",
            Codec.optionalString(definition.description()), "expires",
            Codec.optionalString(definition.expires())
        ));
    }

    static FeatureFlag.Definition featureFlagDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new FeatureFlag.Definition(
            Codec.stringFromJson(object.get("name")),
            FeatureFlag.Type.valueOf(Codec.stringFromJson(object.get("type"))),
            featureFlagValueFromJson(object.get("default_value")),
            FeatureFlag.ScopeKind.valueOf(Codec.stringFromJson(object.get("scope"))),
            Codec.optionalStringFromJson(object.get("owner")),
            Codec.optionalStringFromJson(object.get("description")),
            Codec.optionalStringFromJson(object.get("expires"))
        );
    }
}
