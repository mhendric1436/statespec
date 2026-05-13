package com.statespec.backend;

import java.math.BigDecimal;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public final class JsonTest {
    public static void main(String[] args) {
        parsesObjectsAndArrays();
        decodesEscapesAndUnicode();
        canonicalStringIsStable();
        rejectsMalformedInput();
        backendSurfacesUseTypedJson();
    }

    private static void parsesObjectsAndArrays() {
        Json parsed = Json.parse("{\"active\":true,\"count\":7,\"tags\":[\"a\",null,2.5]}");

        Json.ObjectValue object = requireType(parsed, Json.ObjectValue.class);
        Json.BooleanValue active = requireType(object.values().get("active"), Json.BooleanValue.class);
        Json.IntegerValue count = requireType(object.values().get("count"), Json.IntegerValue.class);
        Json.ArrayValue tags = requireType(object.values().get("tags"), Json.ArrayValue.class);
        Json.DecimalValue decimal = requireType(tags.values().get(2), Json.DecimalValue.class);

        require(active.value(), "active should be true");
        require(count.value() == 7, "count should be 7");
        require(tags.values().size() == 3, "tags should have three values");
        require(decimal.value().compareTo(new BigDecimal("2.5")) == 0, "decimal should be 2.5");
    }

    private static void decodesEscapesAndUnicode() {
        Json parsed = Json.parse("{\"text\":\"line\\nA\\\\\\\"\",\"latin\":\"\\u00e9\",\"emoji\":\"\\ud83d\\ude00\"}");
        Json.ObjectValue object = requireType(parsed, Json.ObjectValue.class);

        require(
            requireType(object.values().get("text"), Json.StringValue.class).value().equals("line\nA\\\""),
            "text escapes should decode"
        );
        require(
            requireType(object.values().get("latin"), Json.StringValue.class).value().equals("é"),
            "latin unicode should decode"
        );
        require(
            requireType(object.values().get("emoji"), Json.StringValue.class).value().equals("😀"),
            "surrogate pair should decode"
        );
    }

    private static void canonicalStringIsStable() {
        Json value = Json.object(Map.of(
            "z", Json.integer(1),
            "a", Json.array(List.of(Json.bool(true), Json.string("x")))
        ));

        String canonical = value.canonicalString();
        require(canonical.equals("{\"a\":[true,\"x\"],\"z\":1}"), "canonical string should sort keys");
        require(Json.parse(canonical).equals(value), "canonical string should round trip");
    }

    private static void rejectsMalformedInput() {
        requireThrows(() -> Json.parse("[1,]"), "trailing comma should fail");
        requireThrows(() -> Json.parse("{\"a\":1,\"a\":2}"), "duplicate key should fail");
        requireThrows(() -> Json.parse("01"), "leading zero should fail");
        requireThrows(() -> Json.parse("-01"), "negative leading zero should fail");
        requireThrows(() -> Json.parse("\"raw\nnewline\""), "raw newline should fail");
        requireThrows(() -> Json.parse("\"\\uD800\""), "missing low surrogate should fail");
        requireThrows(() -> Json.parse("\"\\uDC00\""), "unexpected low surrogate should fail");
    }

    private static void backendSurfacesUseTypedJson() {
        Json document = Json.object(Map.of("id", Json.string("user:1"), "active", Json.bool(true)));
        Backend.VersionedRecord record = new Backend.VersionedRecord("users", "user:1", 3L, document);
        require(record.document().find("id").isPresent(), "versioned record document should be typed JSON");

        Backend.Query.JsonEquals query = new Backend.Query.JsonEquals("$.active", Json.bool(true));
        require(query.value() instanceof Json.BooleanValue, "JSON equality value should be typed JSON");

        Backend.IndexValue.StringValue indexValue = new Backend.IndexValue.StringValue("alice@example.com");
        require(indexValue.value() instanceof Json.StringValue, "index value should expose typed JSON");
    }

    private static <T> T requireType(Object value, Class<T> type) {
        if (!type.isInstance(value)) {
            throw new AssertionError("expected " + type.getSimpleName() + " but got " + value);
        }
        return type.cast(value);
    }

    private static void require(boolean condition, String message) {
        if (!condition) {
            throw new AssertionError(message);
        }
    }

    private static void requireThrows(Runnable runnable, String message) {
        try {
            runnable.run();
        } catch (IllegalArgumentException expected) {
            return;
        }
        throw new AssertionError(message);
    }
}
