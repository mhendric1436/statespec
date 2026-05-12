#include "statespec/generator_rust.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace statespec
{

namespace
{

std::string read_template_file(
    const std::filesystem::path& path,
    DiagnosticBag& diagnostics
)
{
    std::ifstream input(path);
    if (!input)
    {
        diagnostics.error(SourceRange{}, "SSPEC5201", "failed to read binding template: " + path.string());
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void add_template_file(
    GenerationResult& result,
    const std::filesystem::path& output_dir,
    const std::filesystem::path& template_path,
    const std::filesystem::path& relative_output_path,
    DiagnosticBag& diagnostics
)
{
    const auto content = read_template_file(template_path, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }

    result.files.push_back(GeneratedFile{
        (output_dir / relative_output_path).string(),
        content,
    });
}

std::string rust_string(const std::string& value)
{
    std::ostringstream out;
    out << '"';
    for (const auto ch : value)
    {
        switch (ch)
        {
        case '"':
            out << "\\\"";
            break;
        case '\\':
            out << "\\\\";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            out << ch;
            break;
        }
    }
    out << '"';
    return out.str();
}

bool is_optional_type(const std::string& type)
{
    return !type.empty() && type.back() == '?';
}

std::string strip_optional_suffix(const std::string& type)
{
    return is_optional_type(type) ? type.substr(0, type.size() - 1) : type;
}

long long parse_duration_seconds(const std::optional<std::string>& value)
{
    if (!value.has_value() || value->empty())
    {
        return 0;
    }
    const auto& text = *value;
    if (text.size() < 3 || text[0] != 'P' || text[1] != 'T')
    {
        return 0;
    }

    long long total = 0;
    long long current = 0;
    for (std::size_t i = 2; i < text.size(); ++i)
    {
        const auto ch = text[i];
        if (std::isdigit(static_cast<unsigned char>(ch)))
        {
            current = current * 10 + static_cast<long long>(ch - '0');
        }
        else
        {
            if (ch == 'H')
            {
                total += current * 3600;
            }
            else if (ch == 'M')
            {
                total += current * 60;
            }
            else if (ch == 'S')
            {
                total += current;
            }
            current = 0;
        }
    }
    return total;
}

std::string optional_string_expr(const std::optional<std::string>& value)
{
    return value.has_value() ? "Some(" + rust_string(*value) + ".to_string())" : "None";
}

std::string optional_duration_expr(const std::optional<std::string>& value)
{
    return value.has_value()
        ? "Some(Duration::from_secs(" + std::to_string(parse_duration_seconds(value)) + "))"
        : "None";
}

std::string generate_descriptors_rs(const Spec& spec)
{
    std::ostringstream out;
    out << "use std::time::Duration;\n\n";
    out << "use crate::backend::{CollectionDescriptor, FieldDescriptor};\n";
    out << "use crate::queue::QueueDefinition;\n";
    out << "use crate::workflow::{WorkflowDefinition, WorkflowStepDefinition};\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct LeaseDefinition {\n";
    out << "    pub name: String,\n";
    out << "    pub resource: Option<String>,\n";
    out << "    pub ttl: Duration,\n";
    out << "    pub renew_every: Option<Duration>,\n";
    out << "    pub holder: Option<String>,\n";
    out << "    pub fencing_token: bool,\n";
    out << "    pub max_ttl: Option<Duration>,\n";
    out << "}\n\n";

    out << "pub fn collection_descriptors() -> Vec<CollectionDescriptor> {\n";
    out << "    vec![\n";

    if (spec.system.has_value())
    {
        for (const auto& entity : spec.system->entities)
        {
            out << "        CollectionDescriptor {\n";
            out << "            name: " << rust_string(entity.name) << ".to_string(),\n";
            out << "            fields: vec![\n";
            for (const auto& field : entity.fields)
            {
                out << "                FieldDescriptor { name: " << rust_string(field.name)
                    << ".to_string(), field_type: " << rust_string(strip_optional_suffix(field.type))
                    << ".to_string(), required: " << (is_optional_type(field.type) ? "false" : "true")
                    << " },\n";
            }
            out << "            ],\n";
            out << "            key_fields: vec![";
            for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(entity.key_fields[i]) << ".to_string()";
            }
            out << "],\n";
            out << "            indexes: vec![],\n";
            out << "            schema_version: 1,\n";
            out << "        },\n";
        }
    }

    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn queue_definitions() -> Vec<QueueDefinition> {\n";
    out << "    vec![\n";
    if (spec.system.has_value())
    {
        for (const auto& queue : spec.system->queues)
        {
            out << "        QueueDefinition {\n";
            out << "            queue: " << rust_string(queue.name) << ".to_string(),\n";
            out << "            channel: " << rust_string(queue.channel.value_or("default")) << ".to_string(),\n";
            out << "            visibility_timeout: Duration::from_secs(" << parse_duration_seconds(queue.visibility_timeout) << "),\n";
            out << "            max_attempts: " << queue.max_attempts.value_or(1) << ",\n";
            out << "            dead_letter_queue: " << optional_string_expr(queue.dead_letter) << ",\n";
            out << "            metadata: \"{}\".to_string(),\n";
            out << "        },\n";
        }
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn lease_definitions() -> Vec<LeaseDefinition> {\n";
    out << "    vec![\n";
    if (spec.system.has_value())
    {
        for (const auto& lease : spec.system->leases)
        {
            out << "        LeaseDefinition {\n";
            out << "            name: " << rust_string(lease.name) << ".to_string(),\n";
            out << "            resource: " << optional_string_expr(lease.resource) << ",\n";
            out << "            ttl: Duration::from_secs(" << parse_duration_seconds(lease.ttl) << "),\n";
            out << "            renew_every: " << optional_duration_expr(lease.renew_every) << ",\n";
            out << "            holder: " << optional_string_expr(lease.holder) << ",\n";
            out << "            fencing_token: " << (lease.fencing_token.value_or(false) ? "true" : "false") << ",\n";
            out << "            max_ttl: " << optional_duration_expr(lease.max_ttl) << ",\n";
            out << "        },\n";
        }
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn workflow_definitions() -> Vec<WorkflowDefinition> {\n";
    out << "    vec![\n";
    if (spec.system.has_value())
    {
        for (const auto& workflow : spec.system->workflows)
        {
            out << "        WorkflowDefinition {\n";
            out << "            workflow_name: " << rust_string(workflow.name) << ".to_string(),\n";
            out << "            workflow_version: " << workflow.version.value_or(1) << ",\n";
            out << "            start_step: " << rust_string(workflow.start_step.value_or("")) << ".to_string(),\n";
            out << "            expected_execution_time: Duration::from_secs(" << parse_duration_seconds(workflow.expected_execution_time) << "),\n";
            out << "            singleton: " << (workflow.singleton.value_or(false) ? "true" : "false") << ",\n";
            out << "            steps: vec![\n";
            for (const auto& step : workflow.steps)
            {
                out << "                WorkflowStepDefinition { name: " << rust_string(step.name)
                    << ".to_string(), expected_execution_time: Duration::from_secs(" << parse_duration_seconds(step.expected_execution_time)
                    << "), max_retries: " << step.max_retries.value_or(0) << " },\n";
            }
            out << "            ],\n";
            out << "            metadata: \"{}\".to_string(),\n";
            out << "        },\n";
        }
    }
    out << "    ]\n";
    out << "}\n";
    return out.str();
}

} // namespace

GenerationResult generate_rust_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/rust"};

    add_template_file(result, options.output_dir, template_root / "backend.rs", "backend.rs", diagnostics);
    add_template_file(result, options.output_dir, template_root / "lease.rs", "lease.rs", diagnostics);
    add_template_file(result, options.output_dir, template_root / "queue.rs", "queue.rs", diagnostics);
    add_template_file(result, options.output_dir, template_root / "workflow.rs", "workflow.rs", diagnostics);

    if (!diagnostics.has_errors())
    {
        result.files.push_back(GeneratedFile{
            (options.output_dir / "descriptors.rs").string(),
            generate_descriptors_rs(spec),
        });
    }

    return result;
}

} // namespace statespec
