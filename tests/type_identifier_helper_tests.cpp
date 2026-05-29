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

TEST_CASE("API codec field helpers use entity constants for durable fields")
{
    statespec::IrEntity entity;
    entity.name = "ProjectTask";
    entity.key_fields = {"tenant_id", "task_id"};
    entity.fields = {
        {"tenant_id", "string"},
        {"task_id", "string"},
        {"display_name", "string"},
        {"status", "string"},
    };

    REQUIRE(
        statespec::cpp_api_codec_field_name_expr(entity, "tenant_id") ==
        "entities::project_task::constants::kProjectTaskFieldTenantId"
    );
    REQUIRE(statespec::cpp_api_codec_field_name_expr(entity, "items") == "\"items\"");

    REQUIRE(
        statespec::go_api_codec_field_name_expr(entity, "display_name") ==
        "entityconstants.ProjectTaskFieldDisplayName"
    );
    REQUIRE(statespec::go_api_codec_field_name_expr(entity, "items") == "\"items\"");

    REQUIRE(
        statespec::java_api_codec_field_name_expr(entity, "status") ==
        "Constants.PROJECT_TASK_FIELD_STATUS"
    );
    REQUIRE(statespec::java_api_codec_field_name_expr(entity, "items") == "\"items\"");

    REQUIRE(
        statespec::rust_api_codec_field_name_expr(entity, "task_id") ==
        "entity_constants::PROJECT_TASK_FIELD_TASK_ID"
    );
    REQUIRE(statespec::rust_api_codec_field_name_expr(entity, "items") == "\"items\"");
}

TEST_CASE("string helpers provide ascii-only predicates")
{
    REQUIRE(statespec::starts_with("optional<string>", "optional<"));
    REQUIRE(statespec::ends_with("optional<string>", ">"));
    REQUIRE(statespec::lower_ascii("GET") == "get");
    REQUIRE(statespec::trim_ascii_copy("  api  ") == "api");
}
