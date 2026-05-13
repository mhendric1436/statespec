#include "catch2/catch_amalgamated.hpp"
#include "../bindings/cpp/backend.hpp"

#include <cstdint>
#include <limits>
#include <string>

TEST_CASE("C++ binding JSON parses objects and arrays")
{
    auto parsed = statespec::backend::Json::parse(
        R"json({"active":true,"count":7,"tags":["a",null,2.5]})json"
    );

    REQUIRE(parsed.is_object());
    REQUIRE(parsed["active"].as_bool());
    REQUIRE(parsed["count"].as_int64() == 7);
    REQUIRE(parsed["tags"].is_array());
    REQUIRE(parsed["tags"].as_array().size() == 3);
    REQUIRE(parsed["tags"].as_array()[1].is_null());
    REQUIRE(parsed["tags"].as_array()[2].as_double() == 2.5);
}

TEST_CASE("C++ binding JSON decodes escapes and unicode")
{
    auto parsed = statespec::backend::Json::parse(
        R"json({"text":"line\nA\\\"","latin":"\u00e9","emoji":"\ud83d\ude00"})json"
    );

    REQUIRE(parsed["text"].as_string() == std::string("line\nA\\\""));
    REQUIRE(parsed["latin"].as_string() == u8"é");
    REQUIRE(parsed["emoji"].as_string() == u8"😀");
}

TEST_CASE("C++ binding JSON canonical string is stable")
{
    auto value = statespec::backend::Json::object(
        {{"z", std::int64_t{1}}, {"a", statespec::backend::Json::array({true, "x"})}}
    );

    auto canonical = value.canonical_string();

    REQUIRE(canonical == R"json({"a":[true,"x"],"z":1})json");

    auto reparsed = statespec::backend::Json::parse(canonical);
    REQUIRE(reparsed == value);
}

TEST_CASE("C++ binding JSON object helpers distinguish missing fields")
{
    auto value = statespec::backend::Json::parse(R"json({"present":null,"name":"Alice"})json");

    REQUIRE(value.contains("present"));
    REQUIRE(value.find("present") != nullptr);
    REQUIRE(value.find("present")->is_null());
    REQUIRE(!value.contains("missing"));
    REQUIRE(value.find("missing") == nullptr);
}

TEST_CASE("C++ binding JSON rejects malformed input")
{
    REQUIRE_THROWS_AS(statespec::backend::Json::parse(""), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("[1,]"), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("{\"a\":1,\"a\":2}"), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("01"), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("-01"), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("\"raw\nnewline\""), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("\"\\uD800\""), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("\"\\uDC00\""), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("1e9999"), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(
        statespec::backend::Json::object({{"a", 1}, {"a", 2}}),
        statespec::backend::JsonError
    );
    REQUIRE_THROWS_AS(
        statespec::backend::Json(std::numeric_limits<double>::infinity()),
        statespec::backend::JsonError
    );
}

TEST_CASE("C++ backend surfaces use typed JSON")
{
    auto document = statespec::backend::Json::object(
        {{"id", "user:1"}, {"active", true}, {"count", std::int64_t{3}}}
    );

    statespec::backend::VersionedRecord record{
        .collection = "users",
        .key = "user:1",
        .version = 3,
        .document = document,
    };

    REQUIRE(record.document.is_object());
    REQUIRE(record.document["id"].as_string() == "user:1");
    REQUIRE(record.document["active"].as_bool());
    REQUIRE(record.document["count"].as_int64() == 3);

    auto query = statespec::backend::Query::json_equals("$.active", statespec::backend::Json(true));
    REQUIRE(query.json_value.has_value());
    REQUIRE(query.json_value->is_bool());
    REQUIRE(query.json_value->as_bool());

    auto string_value = statespec::backend::IndexValue::string_value("alice@example.com");
    auto integer_value = statespec::backend::IndexValue::integer_value(42);
    auto decimal_value = statespec::backend::IndexValue::decimal_value(1.5);
    auto boolean_value = statespec::backend::IndexValue::boolean_value(false);
    auto timestamp_value = statespec::backend::IndexValue::timestamp_value("2026-05-12T00:00:00Z");

    REQUIRE(string_value.value.as_string() == "alice@example.com");
    REQUIRE(integer_value.value.as_int64() == 42);
    REQUIRE(decimal_value.value.as_double() == 1.5);
    REQUIRE(!boolean_value.value.as_bool());
    REQUIRE(timestamp_value.value.as_string() == "2026-05-12T00:00:00Z");
}
