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

std::string generate_descriptors_rs(const IrSystem& system)
{
    std::ostringstream out;
    out << "use std::time::Duration;\n\n";
    out << "use crate::backend::{Backend, BackendResult, CollectionDescriptor, FieldDescriptor, "
           "IndexDescriptor};\n";
    out << "use crate::log::{LogDefinition as RuntimeLogDefinition, LogLevel, LogSink};\n";
    out << "use crate::metric::{MetricDefinition as RuntimeMetricDefinition, MetricKind, "
           "MetricSink};\n";
    out << "use crate::queue::QueueDefinition;\n";
    out << "use crate::workflow::{RegisterWorkflowDefinitionRequest, WorkflowDefinition, "
           "WorkflowStepDefinition, WorkflowStore};\n\n";
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
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct FeatureFlagDefinition {\n";
    out << "    pub name: String,\n";
    out << "    pub flag_type: String,\n";
    out << "    pub default_value: String,\n";
    out << "    pub scope: String,\n";
    out << "    pub owner: Option<String>,\n";
    out << "    pub description: Option<String>,\n";
    out << "    pub expires: Option<String>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ShapeDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub fields: Vec<FieldDescriptor>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct LogDefinition {\n";
    out << "    pub name: String,\n";
    out << "    pub level: String,\n";
    out << "    pub event_name: String,\n";
    out << "    pub fields: Vec<FieldDescriptor>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct MetricDefinition {\n";
    out << "    pub name: String,\n";
    out << "    pub kind: String,\n";
    out << "    pub backend_name: String,\n";
    out << "    pub unit: String,\n";
    out << "    pub labels: Vec<FieldDescriptor>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct GarbageCollectionPolicy {\n";
    out << "    pub after: String,\n";
    out << "    pub mode: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityStateDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub terminal: bool,\n";
    out << "    pub garbage_collection: Option<GarbageCollectionPolicy>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityOwnershipDescriptor {\n";
    out << "    pub authority: String,\n";
    out << "    pub system_of_record: String,\n";
    out << "    pub lifecycle: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityRelationDescriptor {\n";
    out << "    pub kind: String,\n";
    out << "    pub name: String,\n";
    out << "    pub target: String,\n";
    out << "    pub optional: bool,\n";
    out << "    pub relation_kind: Option<String>,\n";
    out << "    pub on_parent_delete: Option<String>,\n";
    out << "    pub parent_must_be_in: Vec<String>,\n";
    out << "    pub unique_within_parent: Vec<String>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityChildDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub target_entity: String,\n";
    out << "    pub relation: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityInvariantDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub expression: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub key_fields: Vec<String>,\n";
    out << "    pub ownership: Option<EntityOwnershipDescriptor>,\n";
    out << "    pub relations: Vec<EntityRelationDescriptor>,\n";
    out << "    pub children: Vec<EntityChildDescriptor>,\n";
    out << "    pub invariants: Vec<EntityInvariantDescriptor>,\n";
    out << "    pub states: Vec<EntityStateDescriptor>,\n";
    out << "    pub initial_state: Option<String>,\n";
    out << "    pub terminal_states: Vec<String>,\n";
    out << "}\n\n";

    out << "pub fn feature_flag_definitions() -> Vec<FeatureFlagDefinition> {\n";
    out << "    vec![\n";
    for (const auto& flag : system.feature_flags)
    {
        out << "        FeatureFlagDefinition {\n";
        out << "            name: " << rust_string(flag.name) << ".to_string(),\n";
        out << "            flag_type: " << rust_string(flag.type) << ".to_string(),\n";
        out << "            default_value: " << rust_string(flag.default_value)
            << ".to_string(),\n";
        out << "            scope: " << rust_string(flag.scope) << ".to_string(),\n";
        out << "            owner: " << optional_string_expr(flag.owner) << ",\n";
        out << "            description: " << optional_string_expr(flag.description) << ",\n";
        out << "            expires: " << optional_string_expr(flag.expires) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn shape_descriptors() -> Vec<ShapeDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& shape : system.shapes)
    {
        out << "        ShapeDescriptor {\n";
        out << "            name: " << rust_string(shape.name) << ".to_string(),\n";
        out << "            fields: vec![\n";
        for (const auto& field : shape.fields)
        {
            out << "                FieldDescriptor { name: " << rust_string(field.name)
                << ".to_string(), field_type: " << rust_string(strip_optional_suffix(field.type))
                << ".to_string(), required: " << (is_optional_type(field.type) ? "false" : "true")
                << " },\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn log_definitions() -> Vec<LogDefinition> {\n";
    out << "    vec![\n";
    for (const auto& log : system.logs)
    {
        out << "        LogDefinition {\n";
        out << "            name: " << rust_string(log.name) << ".to_string(),\n";
        out << "            level: " << rust_string(log.level) << ".to_string(),\n";
        out << "            event_name: " << rust_string(log.event_name) << ".to_string(),\n";
        out << "            fields: vec![\n";
        for (const auto& field : log.fields)
        {
            out << "                FieldDescriptor { name: " << rust_string(field.name)
                << ".to_string(), field_type: " << rust_string(strip_optional_suffix(field.type))
                << ".to_string(), required: " << (is_optional_type(field.type) ? "false" : "true")
                << " },\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn metric_definitions() -> Vec<MetricDefinition> {\n";
    out << "    vec![\n";
    for (const auto& metric : system.metrics)
    {
        out << "        MetricDefinition {\n";
        out << "            name: " << rust_string(metric.name) << ".to_string(),\n";
        out << "            kind: " << rust_string(metric.kind) << ".to_string(),\n";
        out << "            backend_name: " << rust_string(metric.backend_name)
            << ".to_string(),\n";
        out << "            unit: " << rust_string(metric.unit) << ".to_string(),\n";
        out << "            labels: vec![\n";
        for (const auto& label : metric.labels)
        {
            out << "                FieldDescriptor { name: " << rust_string(label.name)
                << ".to_string(), field_type: " << rust_string(strip_optional_suffix(label.type))
                << ".to_string(), required: " << (is_optional_type(label.type) ? "false" : "true")
                << " },\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn entity_descriptors() -> Vec<EntityDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& entity : system.entities)
    {
        out << "        EntityDescriptor {\n";
        out << "            name: " << rust_string(entity.name) << ".to_string(),\n";
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
        if (entity.ownership.has_value())
        {
            out << "            ownership: Some(EntityOwnershipDescriptor {\n";
            out << "                authority: " << rust_string(entity.ownership->authority)
                << ".to_string(),\n";
            out << "                system_of_record: "
                << rust_string(entity.ownership->system_of_record) << ".to_string(),\n";
            out << "                lifecycle: " << rust_string(entity.ownership->lifecycle)
                << ".to_string(),\n";
            out << "            }),\n";
        }
        else
        {
            out << "            ownership: None,\n";
        }
        out << "            relations: vec![\n";
        for (const auto& relation : entity.relations)
        {
            out << "                EntityRelationDescriptor {\n";
            out << "                    kind: " << rust_string(relation.kind) << ".to_string(),\n";
            out << "                    name: " << rust_string(relation.name) << ".to_string(),\n";
            out << "                    target: " << rust_string(relation.target)
                << ".to_string(),\n";
            out << "                    optional: " << (relation.optional ? "true" : "false")
                << ",\n";
            out << "                    relation_kind: "
                << optional_string_expr(relation.relation_kind) << ",\n";
            out << "                    on_parent_delete: "
                << optional_string_expr(relation.on_parent_delete) << ",\n";
            out << "                    parent_must_be_in: vec![";
            for (std::size_t i = 0; i < relation.parent_must_be_in.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(relation.parent_must_be_in[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                    unique_within_parent: vec![";
            for (std::size_t i = 0; i < relation.unique_within_parent.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(relation.unique_within_parent[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                },\n";
        }
        out << "            ],\n";
        out << "            children: vec![\n";
        for (const auto& child : entity.children)
        {
            out << "                EntityChildDescriptor { name: " << rust_string(child.name)
                << ".to_string(), target_entity: " << rust_string(child.target_entity)
                << ".to_string(), relation: " << rust_string(child.relation) << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            invariants: vec![\n";
        for (const auto& invariant : entity.invariants)
        {
            out << "                EntityInvariantDescriptor { name: "
                << rust_string(invariant.name)
                << ".to_string(), expression: " << rust_string(invariant.expression)
                << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            states: vec![\n";
        for (const auto& state : entity.states)
        {
            out << "                EntityStateDescriptor {\n";
            out << "                    name: " << rust_string(state.name) << ".to_string(),\n";
            out << "                    terminal: " << (state.terminal ? "true" : "false") << ",\n";
            if (state.garbage_collection.has_value())
            {
                out << "                    garbage_collection: Some(GarbageCollectionPolicy { "
                       "after: "
                    << rust_string(state.garbage_collection->after)
                    << ".to_string(), mode: " << rust_string(state.garbage_collection->mode)
                    << ".to_string() }),\n";
            }
            else
            {
                out << "                    garbage_collection: None,\n";
            }
            out << "                },\n";
        }
        out << "            ],\n";
        out << "            initial_state: " << optional_string_expr(entity.initial_state) << ",\n";
        out << "            terminal_states: vec![";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(entity.terminal_states[i]) << ".to_string()";
        }
        out << "],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn collection_descriptors() -> Vec<CollectionDescriptor> {\n";
    out << "    vec![\n";

    for (const auto& entity : system.entities)
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
        out << "            indexes: vec![\n";
        for (const auto& index : entity.indexes)
        {
            out << "                IndexDescriptor {\n";
            out << "                    name: " << rust_string(index.name) << ".to_string(),\n";
            out << "                    fields: vec![";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(index.fields[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                    unique: " << (index.unique ? "true" : "false") << ",\n";
            out << "                },\n";
        }
        out << "            ],\n";
        out << "            schema_version: 1,\n";
        out << "        },\n";
    }

    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn queue_definitions() -> Vec<QueueDefinition> {\n";
    out << "    vec![\n";
    for (const auto& queue : system.queues)
    {
        out << "        QueueDefinition {\n";
        out << "            queue: " << rust_string(queue.name) << ".to_string(),\n";
        out << "            channel: " << rust_string(queue.channel.value_or("default"))
            << ".to_string(),\n";
        out << "            visibility_timeout: Duration::from_secs("
            << parse_duration_seconds(queue.visibility_timeout) << "),\n";
        out << "            max_attempts: " << queue.max_attempts.value_or(1) << ",\n";
        out << "            dead_letter_queue: " << optional_string_expr(queue.dead_letter)
            << ",\n";
        out << "            metadata: \"{}\".to_string(),\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn lease_definitions() -> Vec<LeaseDefinition> {\n";
    out << "    vec![\n";
    for (const auto& lease : system.leases)
    {
        out << "        LeaseDefinition {\n";
        out << "            name: " << rust_string(lease.name) << ".to_string(),\n";
        out << "            resource: " << optional_string_expr(lease.resource) << ",\n";
        out << "            ttl: Duration::from_secs(" << parse_duration_seconds(lease.ttl)
            << "),\n";
        out << "            renew_every: " << optional_duration_expr(lease.renew_every) << ",\n";
        out << "            holder: " << optional_string_expr(lease.holder) << ",\n";
        out << "            fencing_token: "
            << (lease.fencing_token.value_or(false) ? "true" : "false") << ",\n";
        out << "            max_ttl: " << optional_duration_expr(lease.max_ttl) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn workflow_definitions() -> Vec<WorkflowDefinition> {\n";
    out << "    vec![\n";
    for (const auto& workflow : system.workflows)
    {
        out << "        WorkflowDefinition {\n";
        out << "            workflow_name: " << rust_string(workflow.name) << ".to_string(),\n";
        out << "            workflow_version: " << workflow.version.value_or(1) << ",\n";
        out << "            start_step: " << rust_string(workflow.start_step.value_or(""))
            << ".to_string(),\n";
        out << "            expected_execution_time: Duration::from_secs("
            << parse_duration_seconds(workflow.expected_execution_time) << "),\n";
        out << "            singleton: " << (workflow.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "            steps: vec![\n";
        for (const auto& step : workflow.steps)
        {
            out << "                WorkflowStepDefinition { name: " << rust_string(step.name)
                << ".to_string(), expected_execution_time: Duration::from_secs("
                << parse_duration_seconds(step.expected_execution_time)
                << "), max_retries: " << step.max_retries.value_or(0) << " },\n";
        }
        out << "            ],\n";
        out << "            metadata: \"{}\".to_string(),\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n";

    out << "\nfn log_level_from_descriptor(level: &str) -> LogLevel {\n";
    out << "    match level {\n";
    out << "        \"debug\" => LogLevel::Debug,\n";
    out << "        \"warn\" => LogLevel::Warn,\n";
    out << "        \"error\" => LogLevel::Error,\n";
    out << "        _ => LogLevel::Info,\n";
    out << "    }\n";
    out << "}\n\n";

    out << "fn metric_kind_from_descriptor(kind: &str) -> MetricKind {\n";
    out << "    match kind {\n";
    out << "        \"gauge\" => MetricKind::Gauge,\n";
    out << "        \"histogram\" => MetricKind::Histogram,\n";
    out << "        _ => MetricKind::Counter,\n";
    out << "    }\n";
    out << "}\n\n";

    out << "pub fn ensure_system_collections<B: Backend>(backend: &B) -> BackendResult<()> {\n";
    out << "    backend.ensure_collections(&collection_descriptors())\n";
    out << "}\n\n";

    out << "pub fn register_log_definitions_tx<B: Backend, S: LogSink<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    sink: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in log_definitions() {\n";
    out << "        sink.register_definition_tx(\n";
    out << "            tx,\n";
    out << "            &RuntimeLogDefinition {\n";
    out << "                name: definition.name,\n";
    out << "                level: log_level_from_descriptor(&definition.level),\n";
    out << "                event_name: definition.event_name,\n";
    out << "                fields: definition.fields,\n";
    out << "            },\n";
    out << "        )?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_metric_definitions_tx<B: Backend, S: MetricSink<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    sink: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in metric_definitions() {\n";
    out << "        sink.register_definition_tx(\n";
    out << "            tx,\n";
    out << "            &RuntimeMetricDefinition {\n";
    out << "                name: definition.name,\n";
    out << "                kind: metric_kind_from_descriptor(&definition.kind),\n";
    out << "                backend_name: definition.backend_name,\n";
    out << "                unit: definition.unit,\n";
    out << "                labels: definition.labels,\n";
    out << "            },\n";
    out << "        )?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_observability_catalog_tx<B, L, M>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    log_sink: &L,\n";
    out << "    metric_sink: &M,\n";
    out << ") -> BackendResult<()>\n";
    out << "where\n";
    out << "    B: Backend,\n";
    out << "    L: LogSink<B>,\n";
    out << "    M: MetricSink<B>,\n";
    out << "{\n";
    out << "    register_log_definitions_tx(tx, log_sink)?;\n";
    out << "    register_metric_definitions_tx(tx, metric_sink)\n";
    out << "}\n\n";

    out << "pub fn register_workflow_definitions_tx<B: Backend, S: WorkflowStore<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    store: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in workflow_definitions() {\n";
    out << "        store.register_definition_tx(\n";
    out << "            tx,\n";
    out << "            &RegisterWorkflowDefinitionRequest { definition },\n";
    out << "        )?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n";
    return out.str();
}

} // namespace

GenerationResult generate_rust_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/rust"};

    add_template_file(
        result, options.output_dir, template_root / "json.rs", "json.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "backend.rs", "backend.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "feature_flag.rs", "feature_flag.rs",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "lease.rs", "lease.rs", diagnostics
    );
    add_template_file(result, options.output_dir, template_root / "log.rs", "log.rs", diagnostics);
    add_template_file(
        result, options.output_dir, template_root / "metric.rs", "metric.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "queue.rs", "queue.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "workflow.rs", "workflow.rs", diagnostics
    );

    if (!diagnostics.has_errors())
    {
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "descriptors.rs").string(),
                generate_descriptors_rs(system),
            }
        );
    }

    return result;
}

} // namespace statespec
