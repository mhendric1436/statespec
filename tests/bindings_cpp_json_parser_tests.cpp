#include "catch2/catch_amalgamated.hpp"
#include "../bindings/cpp/json.hpp"

#include <cstdint>
#include <string>

TEST_CASE("C++ binding JSON parser parses canonical objects and arrays")
{
    auto parsed = statespec::backend::Json::parse(
        R"json({"active":true,"count":7,"name":"Alice","nested":{"ok":false},"tags":["one",null,2.5]})json"
    );

    REQUIRE(parsed.is_object());
    REQUIRE(parsed["active"].as_bool());
    REQUIRE(parsed["count"].as_int64() == 7);
    REQUIRE(parsed["name"].as_string() == "Alice");
    REQUIRE(!parsed["nested"]["ok"].as_bool());
    REQUIRE(parsed["tags"].is_array());
    REQUIRE(parsed["tags"].as_array().size() == 3);
    REQUIRE(parsed["tags"].as_array()[1].is_null());
    REQUIRE(parsed["tags"].as_array()[2].as_double() == 2.5);
}

TEST_CASE("C++ binding JSON parser decodes escapes and unicode")
{
    auto parsed = statespec::backend::Json::parse(
        R"json({"ascii":"line\nA\\\"","latin":"\u00e9","emoji":"\ud83d\ude00"})json"
    );

    REQUIRE(parsed["ascii"].as_string() == std::string("line\nA\\\""));
    REQUIRE(parsed["latin"].as_string() == u8"é");
    REQUIRE(parsed["emoji"].as_string() == u8"😀");
}

TEST_CASE("C++ binding JSON canonical strings are stable")
{
    auto value = statespec::backend::Json::object(
        {{"z", std::int64_t{1}}, {"a", statespec::backend::Json::array({true, "x"})}}
    );

    REQUIRE(value.canonical_string() == R"json({"a":[true,"x"],"z":1})json");

    auto reparsed = statespec::backend::Json::parse(value.canonical_string());
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

TEST_CASE("C++ binding JSON parser rejects malformed input")
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
}

TEST_CASE("C++ binding JSON constructors reject invalid values")
{
    REQUIRE_THROWS_AS(
        statespec::backend::Json::object({{"a", 1}, {"a", 2}}),
        statespec::backend::JsonError
    );
    REQUIRE_THROWS_AS(
        statespec::backend::Json(std::numeric_limits<double>::infinity()),
        statespec::backend::JsonError
    );
}
