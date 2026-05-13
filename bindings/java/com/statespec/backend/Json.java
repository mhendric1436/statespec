package com.statespec.backend;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.TreeMap;

/**
 * Typed JSON value used by the Java backend binding.
 *
 * <p>The backend abstraction should carry this value instead of raw JSON strings so
 * binding implementations can validate, inspect, and serialize documents
 * deterministically.</p>
 */
public sealed interface Json permits Json.NullValue, Json.BooleanValue, Json.IntegerValue,
        Json.DecimalValue, Json.StringValue, Json.ArrayValue, Json.ObjectValue {

    record NullValue() implements Json {}

    record BooleanValue(boolean value) implements Json {}

    record IntegerValue(long value) implements Json {}

    record DecimalValue(BigDecimal value) implements Json {
        public DecimalValue {
            Objects.requireNonNull(value, "value");
        }
    }

    record StringValue(String value) implements Json {
        public StringValue {
            Objects.requireNonNull(value, "value");
        }
    }

    record ArrayValue(List<Json> values) implements Json {
        public ArrayValue {
            values = List.copyOf(values);
        }
    }

    record ObjectValue(Map<String, Json> values) implements Json {
        public ObjectValue {
            values = Collections.unmodifiableMap(new TreeMap<>(values));
        }
    }

    static Json nullValue() {
        return new NullValue();
    }

    static Json bool(boolean value) {
        return new BooleanValue(value);
    }

    static Json integer(long value) {
        return new IntegerValue(value);
    }

    static Json decimal(BigDecimal value) {
        return new DecimalValue(value);
    }

    static Json decimal(String value) {
        return decimal(new BigDecimal(value));
    }

    static Json string(String value) {
        return new StringValue(value);
    }

    static Json array(List<Json> values) {
        return new ArrayValue(values);
    }

    static Json object(Map<String, Json> values) {
        return new ObjectValue(values);
    }

    static Json parse(String encoded) {
        return new Parser(encoded).parse();
    }

    default Optional<Json> find(String key) {
        if (this instanceof ObjectValue object) {
            return Optional.ofNullable(object.values().get(key));
        }
        return Optional.empty();
    }

    default String canonicalString() {
        StringBuilder builder = new StringBuilder();
        appendCanonical(builder, this);
        return builder.toString();
    }

    private static void appendCanonical(StringBuilder builder, Json value) {
        switch (value) {
            case NullValue ignored -> builder.append("null");
            case BooleanValue bool -> builder.append(bool.value() ? "true" : "false");
            case IntegerValue integer -> builder.append(integer.value());
            case DecimalValue decimal -> {
                String encoded = decimal.value().stripTrailingZeros().toPlainString();
                if (!encoded.contains(".") && !encoded.contains("E") && !encoded.contains("e")) {
                    encoded += ".0";
                }
                builder.append(encoded);
            }
            case StringValue string -> appendEscapedString(builder, string.value());
            case ArrayValue array -> {
                builder.append('[');
                boolean first = true;
                for (Json item : array.values()) {
                    if (!first) {
                        builder.append(',');
                    }
                    first = false;
                    appendCanonical(builder, item);
                }
                builder.append(']');
            }
            case ObjectValue object -> {
                builder.append('{');
                boolean first = true;
                for (Map.Entry<String, Json> entry : object.values().entrySet()) {
                    if (!first) {
                        builder.append(',');
                    }
                    first = false;
                    appendEscapedString(builder, entry.getKey());
                    builder.append(':');
                    appendCanonical(builder, entry.getValue());
                }
                builder.append('}');
            }
        }
    }

    private static void appendEscapedString(StringBuilder builder, String value) {
        builder.append('"');
        for (int offset = 0; offset < value.length();) {
            int codePoint = value.codePointAt(offset);
            offset += Character.charCount(codePoint);
            switch (codePoint) {
                case '"' -> builder.append("\\\"");
                case '\\' -> builder.append("\\\\");
                case '\b' -> builder.append("\\b");
                case '\f' -> builder.append("\\f");
                case '\n' -> builder.append("\\n");
                case '\r' -> builder.append("\\r");
                case '\t' -> builder.append("\\t");
                default -> {
                    if (codePoint < 0x20) {
                        builder.append(String.format("\\u%04x", codePoint));
                    } else {
                        builder.appendCodePoint(codePoint);
                    }
                }
            }
        }
        builder.append('"');
    }

    final class Parser {
        private final String input;
        private int position;

        Parser(String input) {
            this.input = Objects.requireNonNull(input, "input");
        }

        Json parse() {
            Json value = parseValue();
            skipWhitespace();
            if (position != input.length()) {
                fail("unexpected trailing JSON content");
            }
            return value;
        }

        private Json parseValue() {
            skipWhitespace();
            if (startsWith("null")) {
                position += 4;
                return Json.nullValue();
            }
            if (startsWith("true")) {
                position += 4;
                return Json.bool(true);
            }
            if (startsWith("false")) {
                position += 5;
                return Json.bool(false);
            }
            if (position >= input.length()) {
                fail("missing JSON value");
            }
            return switch (input.charAt(position)) {
                case '"' -> Json.string(parseString());
                case '[' -> parseArray();
                case '{' -> parseObject();
                default -> parseNumber();
            };
        }

        private Json parseArray() {
            expect('[');
            List<Json> values = new ArrayList<>();
            if (consume(']')) {
                return Json.array(values);
            }
            while (true) {
                values.add(parseValue());
                if (consume(']')) {
                    return Json.array(values);
                }
                expect(',');
            }
        }

        private Json parseObject() {
            expect('{');
            Map<String, Json> values = new TreeMap<>();
            if (consume('}')) {
                return Json.object(values);
            }
            while (true) {
                skipWhitespace();
                if (position >= input.length() || input.charAt(position) != '"') {
                    fail("JSON object member name must be a string");
                }
                String key = parseString();
                expect(':');
                if (values.containsKey(key)) {
                    fail("duplicate JSON object member");
                }
                values.put(key, parseValue());
                if (consume('}')) {
                    return Json.object(values);
                }
                expect(',');
            }
        }

        private Json parseNumber() {
            int start = position;
            if (peek('-')) {
                position++;
            }
            if (position >= input.length()) {
                fail("invalid JSON number");
            }
            if (peek('0')) {
                position++;
                if (position < input.length() && Character.isDigit(input.charAt(position))) {
                    fail("invalid JSON number leading zero");
                }
            } else if (input.charAt(position) >= '1' && input.charAt(position) <= '9') {
                while (position < input.length() && Character.isDigit(input.charAt(position))) {
                    position++;
                }
            } else {
                fail("invalid JSON number");
            }

            boolean decimal = false;
            if (peek('.')) {
                decimal = true;
                position++;
                if (position >= input.length() || !Character.isDigit(input.charAt(position))) {
                    fail("invalid JSON number fraction");
                }
                while (position < input.length() && Character.isDigit(input.charAt(position))) {
                    position++;
                }
            }

            if (position < input.length() && (input.charAt(position) == 'e' || input.charAt(position) == 'E')) {
                decimal = true;
                position++;
                if (position < input.length() && (input.charAt(position) == '+' || input.charAt(position) == '-')) {
                    position++;
                }
                if (position >= input.length() || !Character.isDigit(input.charAt(position))) {
                    fail("invalid JSON number exponent");
                }
                while (position < input.length() && Character.isDigit(input.charAt(position))) {
                    position++;
                }
            }

            String text = input.substring(start, position);
            if (decimal) {
                return Json.decimal(new BigDecimal(text));
            }
            try {
                return Json.integer(new BigInteger(text).longValueExact());
            } catch (ArithmeticException ex) {
                fail("invalid JSON integer");
                return Json.nullValue();
            }
        }

        private String parseString() {
            expect('"');
            StringBuilder builder = new StringBuilder();
            while (position < input.length()) {
                char c = input.charAt(position++);
                if (c == '"') {
                    return builder.toString();
                }
                if (c < 0x20) {
                    fail("unescaped control character in JSON string");
                }
                if (c != '\\') {
                    builder.append(c);
                    continue;
                }
                if (position >= input.length()) {
                    fail("unterminated JSON escape");
                }
                char escaped = input.charAt(position++);
                switch (escaped) {
                    case '"', '\\', '/' -> builder.append(escaped);
                    case 'b' -> builder.append('\b');
                    case 'f' -> builder.append('\f');
                    case 'n' -> builder.append('\n');
                    case 'r' -> builder.append('\r');
                    case 't' -> builder.append('\t');
                    case 'u' -> appendUnicodeEscape(builder);
                    default -> fail("invalid JSON escape");
                }
            }
            fail("unterminated JSON string");
            return "";
        }

        private void appendUnicodeEscape(StringBuilder builder) {
            int codePoint = parseHexQuad();
            if (codePoint >= 0xD800 && codePoint <= 0xDBFF) {
                if (position + 6 > input.length() || input.charAt(position) != '\\' || input.charAt(position + 1) != 'u') {
                    fail("missing JSON low surrogate");
                }
                position += 2;
                int low = parseHexQuad();
                if (low < 0xDC00 || low > 0xDFFF) {
                    fail("invalid JSON low surrogate");
                }
                codePoint = 0x10000 + (((codePoint - 0xD800) << 10) | (low - 0xDC00));
            } else if (codePoint >= 0xDC00 && codePoint <= 0xDFFF) {
                fail("unexpected JSON low surrogate");
            }
            builder.appendCodePoint(codePoint);
        }

        private int parseHexQuad() {
            if (position + 4 > input.length()) {
                fail("short JSON unicode escape");
            }
            int value = 0;
            for (int i = 0; i < 4; ++i) {
                value = (value << 4) | hexValue(input.charAt(position++));
            }
            return value;
        }

        private int hexValue(char c) {
            if (c >= '0' && c <= '9') {
                return c - '0';
            }
            if (c >= 'a' && c <= 'f') {
                return c - 'a' + 10;
            }
            if (c >= 'A' && c <= 'F') {
                return c - 'A' + 10;
            }
            fail("invalid JSON unicode escape");
            return 0;
        }

        private boolean consume(char expected) {
            skipWhitespace();
            if (peek(expected)) {
                position++;
                return true;
            }
            return false;
        }

        private void expect(char expected) {
            if (!consume(expected)) {
                fail("unexpected JSON token");
            }
        }

        private boolean startsWith(String value) {
            return input.startsWith(value, position);
        }

        private boolean peek(char expected) {
            return position < input.length() && input.charAt(position) == expected;
        }

        private void skipWhitespace() {
            while (position < input.length()) {
                char c = input.charAt(position);
                if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                    return;
                }
                position++;
            }
        }

        private void fail(String message) {
            throw new IllegalArgumentException(message);
        }
    }
}
