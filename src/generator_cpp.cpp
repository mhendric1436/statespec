#include "statespec/generator_cpp.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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
        diagnostics.error(
            SourceRange{}, "SSPEC5201", "failed to read binding template: " + path.string()
        );
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

    result.files.push_back(
        GeneratedFile{
            (output_dir / relative_output_path).string(),
            content,
        }
    );
}

std::string cpp_string(const std::string& value)
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

std::int64_t parse_duration_seconds(const std::optional<std::string>& value)
{
    if (!value.has_value() || value->empty())
    {
        return 0;
    }

    const auto& text = *value;
    if (text.size() >= 3 && text[0] == 'P' && text[1] == 'T')
    {
        std::int64_t total = 0;
        std::int64_t current = 0;
        for (std::size_t i = 2; i < text.size(); ++i)
        {
            const auto ch = text[i];
            if (std::isdigit(static_cast<unsigned char>(ch)))
            {
                current = current * 10 + static_cast<std::int64_t>(ch - '0');
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

    return 0;
}

std::string optional_string_expr(const std::optional<std::string>& value)
{
    if (!value.has_value())
    {
        return "std::nullopt";
    }
    return "std::optional<std::string>{" + cpp_string(*value) + "}";
}

std::string generate_system_descriptors_header(const IrSystem& system)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"backend.hpp\"\n";
    out << "#include \"log.hpp\"\n";
    out << "#include \"metric.hpp\"\n";
    out << "#include \"queue.hpp\"\n";
    out << "#include \"workflow.hpp\"\n\n";
    out << "#include <chrono>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n";
    out << "#include <string_view>\n";
    out << "#include <vector>\n\n";
    out << "namespace statespec_generated\n";
    out << "{\n\n";
    out << "struct LeaseDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> resource;\n";
    out << "    std::chrono::seconds ttl;\n";
    out << "    std::optional<std::chrono::seconds> renew_every;\n";
    out << "    std::optional<std::string> holder;\n";
    out << "    bool fencing_token = false;\n";
    out << "    std::optional<std::chrono::seconds> max_ttl;\n";
    out << "};\n\n";
    out << "struct FeatureFlagDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string type;\n";
    out << "    std::string default_value;\n";
    out << "    std::string scope;\n";
    out << "    std::optional<std::string> owner;\n";
    out << "    std::optional<std::string> description;\n";
    out << "    std::optional<std::string> expires;\n";
    out << "};\n\n";

    out << "struct ShapeDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> fields;\n";
    out << "};\n\n";

    out << "struct LogDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string level;\n";
    out << "    std::string event_name;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> fields;\n";
    out << "};\n\n";

    out << "struct MetricDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string kind;\n";
    out << "    std::string backend_name;\n";
    out << "    std::string unit;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> labels;\n";
    out << "};\n\n";

    out << "struct GarbageCollectionPolicy\n";
    out << "{\n";
    out << "    std::string after;\n";
    out << "    std::string mode;\n";
    out << "};\n\n";

    out << "struct EntityStateDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    bool terminal = false;\n";
    out << "    std::optional<GarbageCollectionPolicy> garbage_collection;\n";
    out << "};\n\n";

    out << "struct EntityDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<std::string> key_fields;\n";
    out << "    std::vector<EntityStateDescriptor> states;\n";
    out << "    std::optional<std::string> initial_state;\n";
    out << "    std::vector<std::string> terminal_states;\n";
    out << "};\n\n";

    out << "inline std::vector<FeatureFlagDefinition> feature_flag_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& flag : system.feature_flags)
    {
        out << "        FeatureFlagDefinition{\n";
        out << "            " << cpp_string(flag.name) << ",\n";
        out << "            " << cpp_string(flag.type) << ",\n";
        out << "            " << cpp_string(flag.default_value) << ",\n";
        out << "            " << cpp_string(flag.scope) << ",\n";
        out << "            " << optional_string_expr(flag.owner) << ",\n";
        out << "            " << optional_string_expr(flag.description) << ",\n";
        out << "            " << optional_string_expr(flag.expires) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<ShapeDescriptor> shape_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& shape : system.shapes)
    {
        out << "        ShapeDescriptor{\n";
        out << "            " << cpp_string(shape.name) << ",\n";
        out << "            {\n";
        for (const auto& field : shape.fields)
        {
            out << "                statespec::backend::FieldDescriptor{" << cpp_string(field.name)
                << ", " << cpp_string(strip_optional_suffix(field.type)) << ", "
                << (is_optional_type(field.type) ? "false" : "true") << "},\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<LogDefinition> log_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& log : system.logs)
    {
        out << "        LogDefinition{\n";
        out << "            " << cpp_string(log.name) << ",\n";
        out << "            " << cpp_string(log.level) << ",\n";
        out << "            " << cpp_string(log.event_name) << ",\n";
        out << "            {\n";
        for (const auto& field : log.fields)
        {
            out << "                statespec::backend::FieldDescriptor{" << cpp_string(field.name)
                << ", " << cpp_string(strip_optional_suffix(field.type)) << ", "
                << (is_optional_type(field.type) ? "false" : "true") << "},\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<MetricDefinition> metric_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& metric : system.metrics)
    {
        out << "        MetricDefinition{\n";
        out << "            " << cpp_string(metric.name) << ",\n";
        out << "            " << cpp_string(metric.kind) << ",\n";
        out << "            " << cpp_string(metric.backend_name) << ",\n";
        out << "            " << cpp_string(metric.unit) << ",\n";
        out << "            {\n";
        for (const auto& label : metric.labels)
        {
            out << "                statespec::backend::FieldDescriptor{" << cpp_string(label.name)
                << ", " << cpp_string(strip_optional_suffix(label.type)) << ", "
                << (is_optional_type(label.type) ? "false" : "true") << "},\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<EntityDescriptor> entity_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& entity : system.entities)
    {
        out << "        EntityDescriptor{\n";
        out << "            " << cpp_string(entity.name) << ",\n";
        out << "            {";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_string(entity.key_fields[i]);
        }
        out << "},\n";
        out << "            {\n";
        for (const auto& state : entity.states)
        {
            out << "                EntityStateDescriptor{\n";
            out << "                    " << cpp_string(state.name) << ",\n";
            out << "                    " << (state.terminal ? "true" : "false") << ",\n";
            if (state.garbage_collection.has_value())
            {
                out << "                    std::optional<GarbageCollectionPolicy>{"
                    << "GarbageCollectionPolicy{" << cpp_string(state.garbage_collection->after)
                    << ", " << cpp_string(state.garbage_collection->mode) << "}},\n";
            }
            else
            {
                out << "                    std::nullopt,\n";
            }
            out << "                },\n";
        }
        out << "            },\n";
        out << "            " << optional_string_expr(entity.initial_state) << ",\n";
        out << "            {";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_string(entity.terminal_states[i]);
        }
        out << "},\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<statespec::backend::CollectionDescriptor> "
           "collection_descriptors()\n";
    out << "{\n";
    out << "    return {\n";

    for (const auto& entity : system.entities)
    {
        out << "        statespec::backend::CollectionDescriptor{\n";
        out << "            " << cpp_string(entity.name) << ",\n";
        out << "            {\n";
        for (const auto& field : entity.fields)
        {
            out << "                statespec::backend::FieldDescriptor{" << cpp_string(field.name)
                << ", " << cpp_string(strip_optional_suffix(field.type)) << ", "
                << (is_optional_type(field.type) ? "false" : "true") << "},\n";
        }
        out << "            },\n";
        out << "            {";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_string(entity.key_fields[i]);
        }
        out << "},\n";
        out << "            {},\n";
        out << "            1,\n";
        out << "        },\n";
    }

    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<statespec::backend::QueueDefinition> queue_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& queue : system.queues)
    {
        out << "        statespec::backend::QueueDefinition{\n";
        out << "            " << cpp_string(queue.name) << ",\n";
        out << "            " << cpp_string(queue.channel.value_or("default")) << ",\n";
        out << "            std::chrono::seconds{"
            << parse_duration_seconds(queue.visibility_timeout) << "},\n";
        out << "            " << queue.max_attempts.value_or(1) << ",\n";
        out << "            " << optional_string_expr(queue.dead_letter) << ",\n";
        out << "            \"{}\",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<LeaseDefinition> lease_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& lease : system.leases)
    {
        out << "        LeaseDefinition{\n";
        out << "            " << cpp_string(lease.name) << ",\n";
        out << "            " << optional_string_expr(lease.resource) << ",\n";
        out << "            std::chrono::seconds{" << parse_duration_seconds(lease.ttl) << "},\n";
        if (lease.renew_every.has_value())
        {
            out << "            std::optional<std::chrono::seconds>{std::chrono::seconds{"
                << parse_duration_seconds(lease.renew_every) << "}},\n";
        }
        else
        {
            out << "            std::nullopt,\n";
        }
        out << "            " << optional_string_expr(lease.holder) << ",\n";
        out << "            " << (lease.fencing_token.value_or(false) ? "true" : "false") << ",\n";
        if (lease.max_ttl.has_value())
        {
            out << "            std::optional<std::chrono::seconds>{std::chrono::seconds{"
                << parse_duration_seconds(lease.max_ttl) << "}},\n";
        }
        else
        {
            out << "            std::nullopt,\n";
        }
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<statespec::backend::WorkflowDefinition> workflow_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& workflow : system.workflows)
    {
        out << "        statespec::backend::WorkflowDefinition{\n";
        out << "            " << cpp_string(workflow.name) << ",\n";
        out << "            " << workflow.version.value_or(1) << ",\n";
        out << "            " << cpp_string(workflow.start_step.value_or("")) << ",\n";
        out << "            std::chrono::seconds{"
            << parse_duration_seconds(workflow.expected_execution_time) << "},\n";
        out << "            " << (workflow.singleton.value_or(false) ? "true" : "false") << ",\n";
        out << "            {\n";
        for (const auto& step : workflow.steps)
        {
            out << "                statespec::backend::WorkflowStepDefinition{"
                << cpp_string(step.name) << ", std::chrono::seconds{"
                << parse_duration_seconds(step.expected_execution_time) << "}, "
                << step.max_retries.value_or(0) << "},\n";
        }
        out << "            },\n";
        out << "            \"{}\",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline statespec::backend::LogLevel log_level_from_string(std::string_view level)\n";
    out << "{\n";
    out << "    if (level == \"debug\") { return statespec::backend::LogLevel::Debug; }\n";
    out << "    if (level == \"warn\") { return statespec::backend::LogLevel::Warn; }\n";
    out << "    if (level == \"error\") { return statespec::backend::LogLevel::Error; }\n";
    out << "    return statespec::backend::LogLevel::Info;\n";
    out << "}\n\n";

    out << "inline statespec::backend::MetricKind metric_kind_from_string(std::string_view kind)\n";
    out << "{\n";
    out << "    if (kind == \"gauge\") { return statespec::backend::MetricKind::Gauge; }\n";
    out << "    if (kind == \"histogram\") { return statespec::backend::MetricKind::Histogram; }\n";
    out << "    return statespec::backend::MetricKind::Counter;\n";
    out << "}\n\n";

    out << "inline void ensure_system_collections(statespec::backend::IBackend& backend)\n";
    out << "{\n";
    out << "    backend.ensure_collections(collection_descriptors());\n";
    out << "}\n\n";

    out << "inline void register_log_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::ILogSink& sink\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : log_definitions())\n";
    out << "    {\n";
    out << "        sink.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::LogDefinition{\n";
    out << "                definition.name,\n";
    out << "                log_level_from_string(definition.level),\n";
    out << "                definition.event_name,\n";
    out << "                definition.fields,\n";
    out << "            }\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void register_metric_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IMetricSink& sink\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : metric_definitions())\n";
    out << "    {\n";
    out << "        sink.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::MetricDefinition{\n";
    out << "                definition.name,\n";
    out << "                metric_kind_from_string(definition.kind),\n";
    out << "                definition.backend_name,\n";
    out << "                definition.unit,\n";
    out << "                definition.labels,\n";
    out << "            }\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void register_observability_catalogTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::ILogSink& log_sink,\n";
    out << "    statespec::backend::IMetricSink& metric_sink\n";
    out << ")\n";
    out << "{\n";
    out << "    register_log_definitionsTx(tx, log_sink);\n";
    out << "    register_metric_definitionsTx(tx, metric_sink);\n";
    out << "}\n\n";

    out << "inline void register_workflow_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IWorkflowStore& workflow_store\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : workflow_definitions())\n";
    out << "    {\n";
    out << "        workflow_store.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::RegisterWorkflowDefinitionRequest{definition}\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "} // namespace statespec_generated\n";
    return out.str();
}

} // namespace

GenerationResult generate_cpp_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/cpp"};

    add_template_file(
        result, options.output_dir, template_root / "json.hpp", "json.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "backend.hpp", "backend.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "feature_flag.hpp", "feature_flag.hpp",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "lease.hpp", "lease.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "log.hpp", "log.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "metric.hpp", "metric.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "queue.hpp", "queue.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "workflow.hpp", "workflow.hpp", diagnostics
    );

    if (!diagnostics.has_errors())
    {
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "system_descriptors.hpp").string(),
                generate_system_descriptors_header(system),
            }
        );
    }

    return result;
}

} // namespace statespec
