#pragma once

#include <string>

namespace statespec
{

struct ChildWorkflowDerivedNames
{
    std::string id_bucket_base;
    std::string pending_bucket;
    std::string creating_bucket;
    std::string succeeded_bucket;
    std::string failed_bucket;
    std::string generate_ids_step;
    std::string create_children_step;
    std::string wait_children_step;
};

inline std::string derive_child_workflow_id_bucket_base(const std::string& child_id_field)
{
    if (child_id_field == "id")
    {
        return "ids";
    }

    constexpr const char* id_suffix = "_id";
    constexpr std::size_t id_suffix_size = 3;
    if (child_id_field.size() > id_suffix_size &&
        child_id_field.compare(child_id_field.size() - id_suffix_size, id_suffix_size, id_suffix) ==
            0)
    {
        return child_id_field.substr(0, child_id_field.size() - id_suffix_size) + "_ids";
    }

    return child_id_field + "_ids";
}

inline ChildWorkflowDerivedNames derive_child_workflow_names(
    const std::string& child_workflow_name,
    const std::string& child_id_field
)
{
    ChildWorkflowDerivedNames names;
    names.id_bucket_base = derive_child_workflow_id_bucket_base(child_id_field);
    names.pending_bucket = "pending_" + names.id_bucket_base;
    names.creating_bucket = "creating_" + names.id_bucket_base;
    names.succeeded_bucket = "succeeded_" + names.id_bucket_base;
    names.failed_bucket = "failed_" + names.id_bucket_base;
    names.generate_ids_step = "generate_" + names.id_bucket_base;
    names.create_children_step = "create_" + child_workflow_name;
    names.wait_children_step = "wait_for_" + child_workflow_name;
    return names;
}

} // namespace statespec
