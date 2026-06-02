#include "generator_workflow_metadata.hpp"

#include "openapi_json.hpp"

#include <sstream>

namespace statespec
{

namespace
{

void append_json_string(
    std::ostringstream& out,
    const std::string& value
)
{
    out << '"' << openapi_json_escape(value) << '"';
}

void append_json_member(
    std::ostringstream& out,
    const std::string& name,
    const std::string& value
)
{
    append_json_string(out, name);
    out << ':';
    append_json_string(out, value);
}

void append_json_optional_member(
    std::ostringstream& out,
    const std::string& name,
    const std::optional<std::string>& value
)
{
    append_json_string(out, name);
    out << ':';
    if (value.has_value())
    {
        append_json_string(out, *value);
    }
    else
    {
        out << "null";
    }
}

void append_child_workflow_metadata(
    std::ostringstream& out,
    const IrWorkflowChildWorkflow& child_workflow
)
{
    out << '{';
    append_json_member(out, "name", child_workflow.name);
    out << ',';
    append_json_optional_member(out, "child_entity", child_workflow.child_entity);
    out << ',';
    append_json_optional_member(out, "child_workflow", child_workflow.child_workflow);
    out << ',';
    append_json_optional_member(out, "child_id_field", child_workflow.child_id_field);
    out << ',';
    append_json_optional_member(out, "child_id_type", child_workflow.child_id_type);
    out << ',';
    append_json_optional_member(out, "parent_ref_field", child_workflow.parent_ref_field);
    out << ',';
    append_json_optional_member(out, "parent_ref_expression", child_workflow.parent_ref_expression);
    out << ',';
    append_json_optional_member(out, "desired_count", child_workflow.desired_count);

    out << ',';
    append_json_string(out, "create_assignments");
    out << ":[";
    for (std::size_t i = 0; i < child_workflow.create_assignments.size(); ++i)
    {
        const auto& assignment = child_workflow.create_assignments[i];
        out << '{';
        append_json_member(out, "field", assignment.name);
        out << ',';
        append_json_member(out, "expression", assignment.expression);
        out << '}';
        if (i + 1 < child_workflow.create_assignments.size())
        {
            out << ',';
        }
    }
    out << ']';

    out << ',';
    append_json_optional_member(out, "success_expression", child_workflow.success_expression);
    out << ',';
    append_json_optional_member(out, "failure_expression", child_workflow.failure_expression);

    out << ',';
    append_json_string(out, "derived_names");
    out << ":{";
    append_json_optional_member(out, "id_bucket_base", child_workflow.id_bucket_base);
    out << ',';
    append_json_optional_member(out, "pending_bucket", child_workflow.pending_bucket);
    out << ',';
    append_json_optional_member(out, "creating_bucket", child_workflow.creating_bucket);
    out << ',';
    append_json_optional_member(out, "succeeded_bucket", child_workflow.succeeded_bucket);
    out << ',';
    append_json_optional_member(out, "failed_bucket", child_workflow.failed_bucket);
    out << ',';
    append_json_optional_member(out, "generate_ids_step", child_workflow.generate_ids_step);
    out << ',';
    append_json_optional_member(out, "create_children_step", child_workflow.create_children_step);
    out << ',';
    append_json_optional_member(out, "wait_children_step", child_workflow.wait_children_step);
    out << '}';

    out << '}';
}

} // namespace

std::string workflow_descriptor_metadata_json(const IrWorkflow& workflow)
{
    if (workflow.child_workflows.empty())
    {
        return "{}";
    }

    std::ostringstream out;
    out << "{\"child_workflows\":[";
    for (std::size_t i = 0; i < workflow.child_workflows.size(); ++i)
    {
        append_child_workflow_metadata(out, workflow.child_workflows[i]);
        if (i + 1 < workflow.child_workflows.size())
        {
            out << ',';
        }
    }
    out << "]}";
    return out.str();
}

} // namespace statespec
