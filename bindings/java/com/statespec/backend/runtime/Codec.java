package com.statespec.backend.runtime;

import com.statespec.backend.Json;
import java.time.Duration;
import java.time.Instant;
import java.util.Optional;

final class Codec
{
    private Codec() {}

    static String definitionKey(Object... parts)
    {
        StringBuilder key = new StringBuilder();
        for (int index = 0; index < parts.length; index++)
        {
            if (index > 0)
            {
                key.append(':');
            }
            key.append(parts[index]);
        }
        return key.toString();
    }

    static Json optionalString(Optional<String> value)
    {
        return value.map(Json::string).orElseGet(Json::nullValue);
    }

    static Optional<String> optionalStringFromJson(Json value)
    {
        if (value instanceof Json.NullValue)
        {
            return Optional.empty();
        }
        return Optional.of(stringFromJson(value));
    }

    static String stringFromJson(Json value)
    {
        return ((Json.StringValue)value).value();
    }

    static long longFromJson(Json value)
    {
        return ((Json.IntegerValue)value).value();
    }

    static int intFromJson(Json value)
    {
        return (int)longFromJson(value);
    }

    static double doubleFromJson(Json value)
    {
        if (value instanceof Json.DecimalValue decimal)
        {
            return decimal.value().doubleValue();
        }
        return longFromJson(value);
    }

    static boolean boolFromJson(Json value)
    {
        return ((Json.BooleanValue)value).value();
    }

    static Json durationToJson(Duration value)
    {
        return Json.integer(value.toNanos());
    }

    static Duration durationFromJson(Json value)
    {
        return Duration.ofNanos(longFromJson(value));
    }

    static Json optionalDuration(Optional<Duration> value)
    {
        return value.map(Codec::durationToJson).orElseGet(Json::nullValue);
    }

    static Optional<Duration> optionalDurationFromJson(Json value)
    {
        if (value instanceof Json.NullValue)
        {
            return Optional.empty();
        }
        return Optional.of(durationFromJson(value));
    }

    static Json instantToJson(Instant value)
    {
        return Json.integer(value.toEpochMilli());
    }

    static Instant instantFromJson(Json value)
    {
        return Instant.ofEpochMilli(longFromJson(value));
    }

    static Json optionalInstant(Optional<Instant> value)
    {
        return value.map(Codec::instantToJson).orElseGet(Json::nullValue);
    }

    static Optional<Instant> optionalInstantFromJson(Json value)
    {
        if (value instanceof Json.NullValue)
        {
            return Optional.empty();
        }
        return Optional.of(instantFromJson(value));
    }
}
