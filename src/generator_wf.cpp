#include "generator_backend.hpp"

namespace statespec::generator_backend
{

namespace
{

std::string generate_wf_manifest(
    const SystemDecl& system,
    const GenerateDecl& declaration
)
{
    std::ostringstream out;
    append_header(out, system, declaration.target);
    out << "workflows:\n";
    for (const auto& workflow : system.workflows)
    {
        out << "  - name: " << workflow.name << "\n";
        out << "    version: " << workflow.version.value_or(0) << "\n";
        out << "    singleton: " << optional_bool_or_empty(workflow.singleton) << "\n";
        out << "    expected_execution_time: "
            << optional_or_empty(workflow.expected_execution_time) << "\n";
        out << "    start: " << optional_or_empty(workflow.start_step) << "\n";
        out << "    steps:\n";
        for (const auto& step : workflow.steps)
        {
            out << "      - name: " << step.name << "\n";
            out << "        expected_execution_time: "
                << optional_or_empty(step.expected_execution_time) << "\n";
            out << "        max_retries: " << step.max_retries.value_or(0) << "\n";
        }
    }
    return out.str();
}

std::string generate_wf_workflows_header()
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include <cstddef>\n";
    out << "#include <string_view>\n\n";
    out << "namespace statespec_generated::wf\n";
    out << "{\n\n";
    out << "struct WorkflowStepMetadata\n";
    out << "{\n";
    out << "    std::string_view name;\n";
    out << "    std::string_view expected_execution_time;\n";
    out << "    int max_retries;\n";
    out << "};\n\n";
    out << "struct WorkflowMetadata\n";
    out << "{\n";
    out << "    std::string_view name;\n";
    out << "    int version;\n";
    out << "    bool singleton;\n";
    out << "    std::string_view expected_execution_time;\n";
    out << "    std::string_view start_step;\n";
    out << "    const WorkflowStepMetadata* steps;\n";
    out << "    std::size_t step_count;\n";
    out << "};\n\n";
    out << "const WorkflowMetadata* workflows();\n";
    out << "std::size_t workflow_count();\n";
    out << "const WorkflowMetadata* find_workflow(std::string_view name);\n";
    out << "const WorkflowStepMetadata* find_step(";
    out << "std::string_view workflow_name, std::string_view step_name);\n";
    out << "std::string_view start_step(std::string_view workflow_name);\n\n";
    out << "} // namespace statespec_generated::wf\n";
    return out.str();
}

std::string generate_wf_metadata_source(const SystemDecl& system)
{
    std::ostringstream out;
    out << "#include \"wf_workflows.hpp\"\n\n";
    out << "#include <array>\n\n";
    out << "namespace statespec_generated::wf\n";
    out << "{\n";
    out << "namespace\n";
    out << "{\n\n";

    for (const auto& workflow : system.workflows)
    {
        const auto symbol = to_lower(workflow.name);
        out << "constexpr std::array<WorkflowStepMetadata, " << workflow.steps.size() << "> "
            << symbol << "_steps{{\n";
        for (const auto& step : workflow.steps)
        {
            out << "    WorkflowStepMetadata{\"" << step.name << "\", \""
                << optional_or_empty(step.expected_execution_time) << "\", "
                << step.max_retries.value_or(0) << "},\n";
        }
        out << "}};\n\n";
    }

    out << "constexpr std::array<WorkflowMetadata, " << system.workflows.size()
        << "> all_workflows{{\n";
    for (const auto& workflow : system.workflows)
    {
        const auto symbol = to_lower(workflow.name);
        out << "    WorkflowMetadata{\"" << workflow.name << "\", " << workflow.version.value_or(0)
            << ", " << bool_text(workflow.singleton.value_or(false)) << ", \""
            << optional_or_empty(workflow.expected_execution_time) << "\", \""
            << optional_or_empty(workflow.start_step) << "\", " << symbol << "_steps.data(), "
            << symbol << "_steps.size()},\n";
    }
    out << "}};\n\n";
    out << "} // namespace\n\n";
    out << "const WorkflowMetadata* workflows()\n";
    out << "{\n";
    out << "    return all_workflows.data();\n";
    out << "}\n\n";
    out << "std::size_t workflow_count()\n";
    out << "{\n";
    out << "    return all_workflows.size();\n";
    out << "}\n\n";
    out << "const WorkflowMetadata* find_workflow(std::string_view name)\n";
    out << "{\n";
    out << "    for (std::size_t i = 0; i < all_workflows.size(); ++i)\n";
    out << "    {\n";
    out << "        if (all_workflows[i].name == name)\n";
    out << "        {\n";
    out << "            return &all_workflows[i];\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return nullptr;\n";
    out << "}\n\n";
    out << "const WorkflowStepMetadata* find_step(";
    out << "std::string_view workflow_name, std::string_view step_name)\n";
    out << "{\n";
    out << "    const auto* workflow = find_workflow(workflow_name);\n";
    out << "    if (workflow == nullptr)\n";
    out << "    {\n";
    out << "        return nullptr;\n";
    out << "    }\n";
    out << "    for (std::size_t i = 0; i < workflow->step_count; ++i)\n";
    out << "    {\n";
    out << "        if (workflow->steps[i].name == step_name)\n";
    out << "        {\n";
    out << "            return &workflow->steps[i];\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return nullptr;\n";
    out << "}\n\n";
    out << "std::string_view start_step(std::string_view workflow_name)\n";
    out << "{\n";
    out << "    const auto* workflow = find_workflow(workflow_name);\n";
    out << "    return workflow == nullptr ? std::string_view{} : workflow->start_step;\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::wf\n";
    return out.str();
}

} // namespace

void generate_wf(
    const SystemDecl& system,
    const GenerateDecl& declaration,
    const GenerationOptions& options,
    GenerationResult& result
)
{
    const auto root = output_root(declaration, options);
    result.files.push_back(
        GeneratedFile{
            join_path(root, "wf-manifest.yaml"), generate_wf_manifest(system, declaration)
        }
    );
    result.files.push_back(
        GeneratedFile{join_path(root, "wf_workflows.hpp"), generate_wf_workflows_header()}
    );
    result.files.push_back(
        GeneratedFile{join_path(root, "wf_metadata.cpp"), generate_wf_metadata_source(system)}
    );
}

} // namespace statespec::generator_backend
