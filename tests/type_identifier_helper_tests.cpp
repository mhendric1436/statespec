#include "../src/generator_cpp_descriptor_support.hpp"
#include "../src/generator_go_descriptor_support.hpp"
#include "../src/generator_java_descriptor_support.hpp"
#include "../src/generator_rust_descriptor_support.hpp"
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

TEST_CASE("entity constant helpers produce language-specific names")
{
    REQUIRE(statespec::cpp_entity_name_constant_name("ProjectTask") == "kProjectTaskEntityName");
    REQUIRE(
        statespec::cpp_entity_field_constant_name("ProjectTask", "tenant_id") ==
        "kProjectTaskFieldTenantId"
    );
    REQUIRE(
        statespec::cpp_entity_index_constant_name("ProjectTask", "by_project_status") ==
        "kProjectTaskIndexByProjectStatus"
    );
    REQUIRE(
        statespec::cpp_entity_state_constant_name("ProjectTask", "InProgress") ==
        "kProjectTaskStatusInProgress"
    );

    REQUIRE(statespec::go_entity_name_constant_name("ProjectTask") == "ProjectTaskEntityName");
    REQUIRE(
        statespec::go_entity_field_constant_name("ProjectTask", "tenant_id") ==
        "ProjectTaskFieldTenantId"
    );
    REQUIRE(
        statespec::go_entity_index_constant_name("ProjectTask", "by_project_status") ==
        "ProjectTaskIndexByProjectStatus"
    );
    REQUIRE(
        statespec::go_entity_state_constant_name("ProjectTask", "InProgress") ==
        "ProjectTaskStatusInProgress"
    );

    REQUIRE(statespec::java_entity_name_constant_name("ProjectTask") == "PROJECT_TASK_ENTITY_NAME");
    REQUIRE(
        statespec::java_entity_field_constant_name("ProjectTask", "tenant_id") ==
        "PROJECT_TASK_FIELD_TENANT_ID"
    );
    REQUIRE(
        statespec::java_entity_index_constant_name("ProjectTask", "by_project_status") ==
        "PROJECT_TASK_INDEX_BY_PROJECT_STATUS"
    );
    REQUIRE(
        statespec::java_entity_state_constant_name("ProjectTask", "InProgress") ==
        "PROJECT_TASK_STATUS_IN_PROGRESS"
    );

    REQUIRE(statespec::rust_entity_name_constant_name("ProjectTask") == "PROJECT_TASK_ENTITY_NAME");
    REQUIRE(
        statespec::rust_entity_field_constant_name("ProjectTask", "tenant_id") ==
        "PROJECT_TASK_FIELD_TENANT_ID"
    );
    REQUIRE(
        statespec::rust_entity_index_constant_name("ProjectTask", "by_project_status") ==
        "PROJECT_TASK_INDEX_BY_PROJECT_STATUS"
    );
    REQUIRE(
        statespec::rust_entity_state_constant_name("ProjectTask", "InProgress") ==
        "PROJECT_TASK_STATUS_IN_PROGRESS"
    );
}

TEST_CASE("string helpers provide ascii-only predicates")
{
    REQUIRE(statespec::starts_with("optional<string>", "optional<"));
    REQUIRE(statespec::ends_with("optional<string>", ">"));
    REQUIRE(statespec::lower_ascii("GET") == "get");
    REQUIRE(statespec::trim_ascii_copy("  api  ") == "api");
}
