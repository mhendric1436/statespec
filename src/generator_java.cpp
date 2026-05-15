#include "statespec/generator_java.hpp"

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

std::string java_string(const std::string& value)
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
    return value.has_value() ? "Optional.of(" + java_string(*value) + ")" : "Optional.empty()";
}

std::string optional_duration_expr(const std::optional<std::string>& value)
{
    return value.has_value() ? "Optional.of(Duration.ofSeconds(" +
                                   std::to_string(parse_duration_seconds(value)) + "L))"
                             : "Optional.empty()";
}

std::string generate_descriptors_java(const IrSystem& system)
{
    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << "import com.statespec.backend.Backend.CollectionDescriptor;\n";
    out << "import com.statespec.backend.Backend.FieldDescriptor;\n";
    out << "import com.statespec.backend.Queue.QueueDefinition;\n";
    out << "import com.statespec.backend.Workflow.WorkflowDefinition;\n";
    out << "import com.statespec.backend.Workflow.WorkflowStepDefinition;\n";
    out << "import java.time.Duration;\n";
    out << "import java.util.List;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class Descriptors {\n";
    out << "    private Descriptors() {}\n\n";
    out << "    public record LeaseDefinition(\n";
    out << "        String name,\n";
    out << "        Optional<String> resource,\n";
    out << "        Duration ttl,\n";
    out << "        Optional<Duration> renewEvery,\n";
    out << "        Optional<String> holder,\n";
    out << "        boolean fencingToken,\n";
    out << "        Optional<Duration> maxTtl\n";
    out << "    ) {}\n\n";
    out << "    public record FeatureFlagDefinition(\n";
    out << "        String name,\n";
    out << "        String type,\n";
    out << "        String defaultValue,\n";
    out << "        String scope,\n";
    out << "        Optional<String> owner,\n";
    out << "        Optional<String> description,\n";
    out << "        Optional<String> expires\n";
    out << "    ) {}\n\n";
    out << "    public record LogDefinition(\n";
    out << "        String name,\n";
    out << "        String level,\n";
    out << "        String eventName,\n";
    out << "        List<FieldDescriptor> fields\n";
    out << "    ) {}\n\n";
    out << "    public record MetricDefinition(\n";
    out << "        String name,\n";
    out << "        String kind,\n";
    out << "        String backendName,\n";
    out << "        String unit,\n";
    out << "        List<FieldDescriptor> labels\n";
    out << "    ) {}\n\n";
    out << "    public record GarbageCollectionPolicy(\n";
    out << "        String after,\n";
    out << "        String mode\n";
    out << "    ) {}\n\n";
    out << "    public record EntityStateDescriptor(\n";
    out << "        String name,\n";
    out << "        boolean terminal,\n";
    out << "        Optional<GarbageCollectionPolicy> garbageCollection\n";
    out << "    ) {}\n\n";
    out << "    public record EntityDescriptor(\n";
    out << "        String name,\n";
    out << "        List<String> keyFields,\n";
    out << "        List<EntityStateDescriptor> states,\n";
    out << "        Optional<String> initialState,\n";
    out << "        List<String> terminalStates\n";
    out << "    ) {}\n\n";

    out << "    public static List<FeatureFlagDefinition> featureFlagDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t i = 0; i < system.feature_flags.size(); ++i)
    {
        const auto& flag = system.feature_flags[i];
        out << "            new FeatureFlagDefinition(\n";
        out << "                " << java_string(flag.name) << ",\n";
        out << "                " << java_string(flag.type) << ",\n";
        out << "                " << java_string(flag.default_value) << ",\n";
        out << "                " << java_string(flag.scope) << ",\n";
        out << "                " << optional_string_expr(flag.owner) << ",\n";
        out << "                " << optional_string_expr(flag.description) << ",\n";
        out << "                " << optional_string_expr(flag.expires) << "\n";
        out << "            )" << (i + 1 < system.feature_flags.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<LogDefinition> logDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t log_index = 0; log_index < system.logs.size(); ++log_index)
    {
        const auto& log = system.logs[log_index];
        out << "            new LogDefinition(\n";
        out << "                " << java_string(log.name) << ",\n";
        out << "                " << java_string(log.level) << ",\n";
        out << "                " << java_string(log.event_name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < log.fields.size(); ++i)
        {
            const auto& field = log.fields[i];
            out << "                    new FieldDescriptor(" << java_string(field.name) << ", "
                << java_string(strip_optional_suffix(field.type)) << ", "
                << (is_optional_type(field.type) ? "false" : "true") << ")";
            out << (i + 1 < log.fields.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (log_index + 1 < system.logs.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<MetricDefinition> metricDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t metric_index = 0; metric_index < system.metrics.size(); ++metric_index)
    {
        const auto& metric = system.metrics[metric_index];
        out << "            new MetricDefinition(\n";
        out << "                " << java_string(metric.name) << ",\n";
        out << "                " << java_string(metric.kind) << ",\n";
        out << "                " << java_string(metric.backend_name) << ",\n";
        out << "                " << java_string(metric.unit) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < metric.labels.size(); ++i)
        {
            const auto& label = metric.labels[i];
            out << "                    new FieldDescriptor(" << java_string(label.name) << ", "
                << java_string(strip_optional_suffix(label.type)) << ", "
                << (is_optional_type(label.type) ? "false" : "true") << ")";
            out << (i + 1 < metric.labels.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (metric_index + 1 < system.metrics.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<EntityDescriptor> entityDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t entity_index = 0; entity_index < system.entities.size(); ++entity_index)
    {
        const auto& entity = system.entities[entity_index];
        out << "            new EntityDescriptor(\n";
        out << "                " << java_string(entity.name) << ",\n";
        out << "                List.of(";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_string(entity.key_fields[i]);
        }
        out << "),\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < entity.states.size(); ++i)
        {
            const auto& state = entity.states[i];
            out << "                    new EntityStateDescriptor(\n";
            out << "                        " << java_string(state.name) << ",\n";
            out << "                        " << (state.terminal ? "true" : "false") << ",\n";
            if (state.garbage_collection.has_value())
            {
                out << "                        Optional.of(new GarbageCollectionPolicy("
                    << java_string(state.garbage_collection->after) << ", "
                    << java_string(state.garbage_collection->mode) << "))\n";
            }
            else
            {
                out << "                        Optional.empty()\n";
            }
            out << "                    )" << (i + 1 < entity.states.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                " << optional_string_expr(entity.initial_state) << ",\n";
        out << "                List.of(";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_string(entity.terminal_states[i]);
        }
        out << ")\n";
        out << "            )" << (entity_index + 1 < system.entities.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<CollectionDescriptor> collectionDescriptors() {\n";
    out << "        return List.of(\n";

    if (!system.entities.empty())
    {
        for (std::size_t entity_index = 0; entity_index < system.entities.size(); ++entity_index)
        {
            const auto& entity = system.entities[entity_index];
            out << "            new CollectionDescriptor(\n";
            out << "                " << java_string(entity.name) << ",\n";
            out << "                List.of(\n";
            for (std::size_t i = 0; i < entity.fields.size(); ++i)
            {
                const auto& field = entity.fields[i];
                out << "                    new FieldDescriptor(" << java_string(field.name) << ", "
                    << java_string(strip_optional_suffix(field.type)) << ", "
                    << (is_optional_type(field.type) ? "false" : "true") << ")";
                out << (i + 1 < entity.fields.size() ? "," : "") << "\n";
            }
            out << "                ),\n";
            out << "                List.of(";
            for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << java_string(entity.key_fields[i]);
            }
            out << "),\n";
            out << "                List.of(),\n";
            out << "                1L\n";
            out << "            )";
            out << (entity_index + 1 < system.entities.size() ? "," : "") << "\n";
        }
    }

    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<QueueDefinition> queueDefinitions() {\n";
    out << "        return List.of(\n";
    if (!system.queues.empty())
    {
        for (std::size_t i = 0; i < system.queues.size(); ++i)
        {
            const auto& queue = system.queues[i];
            out << "            new QueueDefinition(\n";
            out << "                " << java_string(queue.name) << ",\n";
            out << "                " << java_string(queue.channel.value_or("default")) << ",\n";
            out << "                Duration.ofSeconds("
                << parse_duration_seconds(queue.visibility_timeout) << "L),\n";
            out << "                " << queue.max_attempts.value_or(1) << ",\n";
            out << "                " << optional_string_expr(queue.dead_letter) << ",\n";
            out << "                \"{}\"\n";
            out << "            )" << (i + 1 < system.queues.size() ? "," : "") << "\n";
        }
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<LeaseDefinition> leaseDefinitions() {\n";
    out << "        return List.of(\n";
    if (!system.leases.empty())
    {
        for (std::size_t i = 0; i < system.leases.size(); ++i)
        {
            const auto& lease = system.leases[i];
            out << "            new LeaseDefinition(\n";
            out << "                " << java_string(lease.name) << ",\n";
            out << "                " << optional_string_expr(lease.resource) << ",\n";
            out << "                Duration.ofSeconds(" << parse_duration_seconds(lease.ttl)
                << "L),\n";
            out << "                " << optional_duration_expr(lease.renew_every) << ",\n";
            out << "                " << optional_string_expr(lease.holder) << ",\n";
            out << "                " << (lease.fencing_token.value_or(false) ? "true" : "false")
                << ",\n";
            out << "                " << optional_duration_expr(lease.max_ttl) << "\n";
            out << "            )" << (i + 1 < system.leases.size() ? "," : "") << "\n";
        }
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<WorkflowDefinition> workflowDefinitions() {\n";
    out << "        return List.of(\n";
    if (!system.workflows.empty())
    {
        for (std::size_t i = 0; i < system.workflows.size(); ++i)
        {
            const auto& workflow = system.workflows[i];
            out << "            new WorkflowDefinition(\n";
            out << "                " << java_string(workflow.name) << ",\n";
            out << "                " << workflow.version.value_or(1) << "L,\n";
            out << "                " << java_string(workflow.start_step.value_or("")) << ",\n";
            out << "                Duration.ofSeconds("
                << parse_duration_seconds(workflow.expected_execution_time) << "L),\n";
            out << "                " << (workflow.singleton.value_or(false) ? "true" : "false")
                << ",\n";
            out << "                List.of(\n";
            for (std::size_t step_index = 0; step_index < workflow.steps.size(); ++step_index)
            {
                const auto& step = workflow.steps[step_index];
                out << "                    new WorkflowStepDefinition(" << java_string(step.name)
                    << ", Duration.ofSeconds("
                    << parse_duration_seconds(step.expected_execution_time) << "L), "
                    << step.max_retries.value_or(0) << ")";
                out << (step_index + 1 < workflow.steps.size() ? "," : "") << "\n";
            }
            out << "                ),\n";
            out << "                \"{}\"\n";
            out << "            )" << (i + 1 < system.workflows.size() ? "," : "") << "\n";
        }
    }
    out << "        );\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

} // namespace

GenerationResult generate_java_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/java/com/statespec/backend"};
    const std::filesystem::path output_root{"com/statespec/backend"};

    add_template_file(
        result, options.output_dir, template_root / "Backend.java", output_root / "Backend.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "Lease.java", output_root / "Lease.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "Log.java", output_root / "Log.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "Metric.java", output_root / "Metric.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "Queue.java", output_root / "Queue.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "Workflow.java", output_root / "Workflow.java",
        diagnostics
    );

    if (!diagnostics.has_errors())
    {
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "com/statespec/generated/Descriptors.java").string(),
                generate_descriptors_java(system),
            }
        );
    }

    return result;
}

} // namespace statespec
