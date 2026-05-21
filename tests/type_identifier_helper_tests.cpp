#include "../src/identifier_case.hpp"
#include "../src/string_utils.hpp"
#include "../src/type_syntax.hpp"

#include "catch2/catch_amalgamated.hpp"

TEST_CASE("type syntax helpers classify optional forms")
{
    REQUIRE(statespec::is_optional_type("string?"));
    REQUIRE(statespec::is_optional_type("optional<string>"));
    REQUIRE_FALSE(statespec::is_optional_type("string"));

    REQUIRE(statespec::strip_optional_type("string?") == "string");
    REQUIRE(statespec::strip_optional_type("optional<string>") == "string");
    REQUIRE(statespec::strip_optional_type("list<string>") == "list<string>");
}

TEST_CASE("identifier helpers normalize generated names")
{
    REQUIRE(statespec::pascal_identifier("external-system profile") == "ExternalSystemProfile");
    REQUIRE(
        statespec::lower_camel_identifier("external-system profile") == "externalSystemProfile"
    );
    REQUIRE(statespec::snake_identifier("ExternalSystemProfile") == "external_system_profile");

    REQUIRE(statespec::pascal_identifier("", "Fallback") == "Fallback");
    REQUIRE(statespec::lower_camel_identifier("", "fallback") == "fallback");
    REQUIRE(statespec::snake_identifier("", "fallback_name") == "fallback_name");
}

TEST_CASE("string helpers provide ascii-only predicates")
{
    REQUIRE(statespec::starts_with("optional<string>", "optional<"));
    REQUIRE(statespec::ends_with("optional<string>", ">"));
    REQUIRE(statespec::lower_ascii("GET") == "get");
    REQUIRE(statespec::trim_ascii_copy("  api  ") == "api");
}
