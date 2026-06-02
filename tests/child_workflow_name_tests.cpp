#include "catch2/catch_amalgamated.hpp"
#include "statespec/child_workflow_names.hpp"

TEST_CASE("child workflow names derive buckets and parent steps from child id field")
{
    const auto names = statespec::derive_child_workflow_names("tasks", "task_id");

    REQUIRE(names.id_bucket_base == "task_ids");
    REQUIRE(names.pending_bucket == "pending_task_ids");
    REQUIRE(names.creating_bucket == "creating_task_ids");
    REQUIRE(names.succeeded_bucket == "succeeded_task_ids");
    REQUIRE(names.failed_bucket == "failed_task_ids");
    REQUIRE(names.generate_ids_step == "generate_task_ids");
    REQUIRE(names.create_children_step == "create_tasks");
    REQUIRE(names.wait_children_step == "wait_for_tasks");
}

TEST_CASE("child workflow id bucket derivation handles generic id fields")
{
    REQUIRE(statespec::derive_child_workflow_id_bucket_base("id") == "ids");
    REQUIRE(statespec::derive_child_workflow_id_bucket_base("child_uuid") == "child_uuid_ids");
}
