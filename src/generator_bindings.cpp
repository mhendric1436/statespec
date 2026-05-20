#include "statespec/generator_bindings.hpp"

#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace statespec
{

namespace
{

std::string trim_copy(const std::string& value)
{
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0)
    {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return value.substr(begin, end - begin);
}

bool starts_with(
    const std::string& value,
    const std::string& prefix
)
{
    return value.rfind(prefix, 0) == 0;
}

bool ends_with(
    const std::string& value,
    const std::string& suffix
)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool validate_binding_generation_request(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    bool valid = true;

    if (!spec.system.has_value())
    {
        diagnostics.error(
            SourceRange{}, "SSPEC5101", "binding generation requires a system declaration"
        );
        valid = false;
    }

    if (options.output_dir.empty())
    {
        diagnostics.error(
            SourceRange{}, "SSPEC5102", "binding generation requires an output directory"
        );
        valid = false;
    }

    if (!options.template_root.empty() && !std::filesystem::exists(options.template_root))
    {
        diagnostics.error(
            SourceRange{}, "SSPEC5103",
            "binding generation template root does not exist: " +
                options.template_root.generic_string()
        );
        valid = false;
    }

    return valid;
}

bool generated_file_selected_for_tier(
    const GeneratedFile& file,
    BindingGenerationTier tier
)
{
    switch (tier)
    {
    case BindingGenerationTier::All:
        return true;
    case BindingGenerationTier::Common:
        return file.tier == GeneratedArtifactTier::Common;
    case BindingGenerationTier::Api:
        return file.tier == GeneratedArtifactTier::Common ||
               file.tier == GeneratedArtifactTier::Api;
    case BindingGenerationTier::Worker:
        return file.tier == GeneratedArtifactTier::Common ||
               file.tier == GeneratedArtifactTier::Worker;
    }
    return true;
}

GenerationResult filter_generation_result_by_tier(
    GenerationResult result,
    BindingGenerationTier tier
)
{
    if (tier == BindingGenerationTier::All)
    {
        return result;
    }

    GenerationResult filtered;
    for (const auto& file : result.files)
    {
        if (generated_file_selected_for_tier(file, tier))
        {
            filtered.files.push_back(file);
        }
    }
    return filtered;
}

} // namespace

BindingGenerationTier parse_binding_generation_tier(const std::string& value)
{
    const auto normalized = trim_copy(value);
    if (normalized == "all")
    {
        return BindingGenerationTier::All;
    }
    if (normalized == "common")
    {
        return BindingGenerationTier::Common;
    }
    if (normalized == "api")
    {
        return BindingGenerationTier::Api;
    }
    if (normalized == "worker")
    {
        return BindingGenerationTier::Worker;
    }
    throw std::invalid_argument(
        "unsupported binding generation tier '" + value +
        "'; supported tiers: " + supported_binding_generation_tiers_text()
    );
}

std::string binding_generation_tier_name(BindingGenerationTier tier)
{
    switch (tier)
    {
    case BindingGenerationTier::All:
        return "all";
    case BindingGenerationTier::Common:
        return "common";
    case BindingGenerationTier::Api:
        return "api";
    case BindingGenerationTier::Worker:
        return "worker";
    }
    return "all";
}

std::string supported_binding_generation_tiers_text()
{
    return "all|common|api|worker";
}

std::string binding_app_artifact_kind_name(BindingAppArtifactKind kind)
{
    switch (kind)
    {
    case BindingAppArtifactKind::ApiApplication:
        return "api_application";
    case BindingAppArtifactKind::ApiServer:
        return "api_server";
    case BindingAppArtifactKind::ApiDispatcher:
        return "api_dispatcher";
    case BindingAppArtifactKind::ApiHandlerRegistry:
        return "api_handler_registry";
    case BindingAppArtifactKind::ApiMain:
        return "api_main";
    case BindingAppArtifactKind::WorkerApplication:
        return "worker_application";
    case BindingAppArtifactKind::WorkerRuntime:
        return "worker_runtime";
    case BindingAppArtifactKind::WorkerRegistry:
        return "worker_registry";
    case BindingAppArtifactKind::WorkflowRunner:
        return "workflow_runner";
    case BindingAppArtifactKind::WorkflowStepHandlers:
        return "workflow_step_handlers";
    case BindingAppArtifactKind::WorkerMain:
        return "worker_main";
    }
    return "api_application";
}

std::vector<BindingAppArtifactModel> binding_app_artifact_model(BindingLanguage language)
{
    using Kind = BindingAppArtifactKind;
    const auto api = GeneratedArtifactTier::Api;
    const auto worker = GeneratedArtifactTier::Worker;

    switch (language)
    {
    case BindingLanguage::Cpp:
        return {
            {"api/api_application.hpp", api, Kind::ApiApplication,
             "API application composition root"},
            {"api/api_server.hpp", api, Kind::ApiServer, "API server lifecycle and request loop",
             true},
            {"api/api_dispatcher.hpp", api, Kind::ApiDispatcher, "API route-to-handler dispatcher",
             true},
            {"api/api_handler_registry.hpp", api, Kind::ApiHandlerRegistry,
             "API handler registry extension point"},
            {"api/main.cpp", api, Kind::ApiMain, "API process entrypoint"},
            {"worker/worker_application.hpp", worker, Kind::WorkerApplication,
             "Worker application composition root", true},
            {"worker/worker_runtime.hpp", worker, Kind::WorkerRuntime,
             "Worker polling and execution runtime"},
            {"worker/worker_registry.hpp", worker, Kind::WorkerRegistry,
             "Worker declaration registry", true},
            {"worker/workflow_runner.hpp", worker, Kind::WorkflowRunner,
             "Workflow claim and step runner", true},
            {"worker/workflow_step_handlers.hpp", worker, Kind::WorkflowStepHandlers,
             "Workflow step handler extension point", true},
            {"worker/main.cpp", worker, Kind::WorkerMain, "Worker process entrypoint"},
        };
    case BindingLanguage::Go:
        return {
            {"api/backend/api_application.go", api, Kind::ApiApplication,
             "API application composition root"},
            {"api/backend/api_server.go", api, Kind::ApiServer,
             "API server lifecycle and request loop", true},
            {"api/backend/api_dispatcher.go", api, Kind::ApiDispatcher,
             "API route-to-handler dispatcher", true},
            {"api/backend/api_handler_registry.go", api, Kind::ApiHandlerRegistry,
             "API handler registry extension point"},
            {"api/cmd/api/main.go", api, Kind::ApiMain, "API process entrypoint"},
            {"worker/backend/worker_application.go", worker, Kind::WorkerApplication,
             "Worker application composition root", true},
            {"worker/backend/worker_runtime.go", worker, Kind::WorkerRuntime,
             "Worker polling and execution runtime"},
            {"worker/backend/worker_registry.go", worker, Kind::WorkerRegistry,
             "Worker declaration registry", true},
            {"worker/backend/workflow_runner.go", worker, Kind::WorkflowRunner,
             "Workflow claim and step runner", true},
            {"worker/backend/workflow_step_handlers.go", worker, Kind::WorkflowStepHandlers,
             "Workflow step handler extension point", true},
            {"worker/cmd/worker/main.go", worker, Kind::WorkerMain, "Worker process entrypoint"},
        };
    case BindingLanguage::Java:
        return {
            {"api/com/statespec/generated/ApiApplication.java", api, Kind::ApiApplication,
             "API application composition root"},
            {"api/com/statespec/generated/ApiServer.java", api, Kind::ApiServer,
             "API server lifecycle and request loop", true},
            {"api/com/statespec/generated/ApiDispatcher.java", api, Kind::ApiDispatcher,
             "API route-to-handler dispatcher", true},
            {"api/com/statespec/generated/ApiHandlerRegistry.java", api, Kind::ApiHandlerRegistry,
             "API handler registry extension point"},
            {"api/com/statespec/generated/ApiMain.java", api, Kind::ApiMain,
             "API process entrypoint"},
            {"worker/com/statespec/generated/WorkerApplication.java", worker,
             Kind::WorkerApplication, "Worker application composition root", true},
            {"worker/com/statespec/generated/WorkerRuntime.java", worker, Kind::WorkerRuntime,
             "Worker polling and execution runtime"},
            {"worker/com/statespec/generated/WorkerRegistry.java", worker, Kind::WorkerRegistry,
             "Worker declaration registry", true},
            {"worker/com/statespec/generated/WorkflowRunner.java", worker, Kind::WorkflowRunner,
             "Workflow claim and step runner", true},
            {"worker/com/statespec/generated/WorkflowStepHandlers.java", worker,
             Kind::WorkflowStepHandlers, "Workflow step handler extension point", true},
            {"worker/com/statespec/generated/WorkerMain.java", worker, Kind::WorkerMain,
             "Worker process entrypoint"},
        };
    case BindingLanguage::Rust:
        return {
            {"api/api_application.rs", api, Kind::ApiApplication,
             "API application composition root"},
            {"api/api_server.rs", api, Kind::ApiServer, "API server lifecycle and request loop",
             true},
            {"api/api_dispatcher.rs", api, Kind::ApiDispatcher, "API route-to-handler dispatcher",
             true},
            {"api/api_handler_registry.rs", api, Kind::ApiHandlerRegistry,
             "API handler registry extension point"},
            {"api/main.rs", api, Kind::ApiMain, "API process entrypoint"},
            {"worker/worker_application.rs", worker, Kind::WorkerApplication,
             "Worker application composition root", true},
            {"worker/worker_runtime.rs", worker, Kind::WorkerRuntime,
             "Worker polling and execution runtime"},
            {"worker/worker_registry.rs", worker, Kind::WorkerRegistry,
             "Worker declaration registry", true},
            {"worker/workflow_runner.rs", worker, Kind::WorkflowRunner,
             "Workflow claim and step runner", true},
            {"worker/workflow_step_handlers.rs", worker, Kind::WorkflowStepHandlers,
             "Workflow step handler extension point", true},
            {"worker/main.rs", worker, Kind::WorkerMain, "Worker process entrypoint"},
        };
    }
    return {};
}

std::filesystem::path default_binding_template_root(BindingLanguage language)
{
    return std::filesystem::path{"bindings"} / to_string(language);
}

std::filesystem::path resolve_binding_template_root(const BindingGeneratorOptions& options)
{
    if (!options.template_root.empty())
    {
        return options.template_root / to_string(options.language);
    }

    if (const char* env_template_root = std::getenv("STATESPEC_TEMPLATE_ROOT");
        env_template_root != nullptr && std::string{env_template_root}.empty() == false)
    {
        return std::filesystem::path{env_template_root} / to_string(options.language);
    }

    return default_binding_template_root(options.language);
}

FieldDescriptorType classify_field_descriptor_type(const std::string& type_name)
{
    const auto normalized = trim_copy(type_name);
    if (normalized == "string")
    {
        return FieldDescriptorType::String;
    }
    if (normalized == "bool")
    {
        return FieldDescriptorType::Bool;
    }
    if (normalized == "int")
    {
        return FieldDescriptorType::Int;
    }
    if (normalized == "int32")
    {
        return FieldDescriptorType::Int32;
    }
    if (normalized == "int64")
    {
        return FieldDescriptorType::Int64;
    }
    if (normalized == "long")
    {
        return FieldDescriptorType::Long;
    }
    if (normalized == "double")
    {
        return FieldDescriptorType::Double;
    }
    if (normalized == "decimal")
    {
        return FieldDescriptorType::Decimal;
    }
    if (normalized == "json")
    {
        return FieldDescriptorType::Json;
    }
    if (normalized == "timestamp")
    {
        return FieldDescriptorType::Timestamp;
    }
    if (normalized == "duration")
    {
        return FieldDescriptorType::Duration;
    }
    if (normalized == "uuid")
    {
        return FieldDescriptorType::Uuid;
    }
    if (ends_with(normalized, "?") || starts_with(normalized, "optional<"))
    {
        return FieldDescriptorType::Optional;
    }
    if (starts_with(normalized, "list<") || ends_with(normalized, "[]"))
    {
        return FieldDescriptorType::List;
    }
    if (starts_with(normalized, "set<"))
    {
        return FieldDescriptorType::Set;
    }
    if (starts_with(normalized, "map<"))
    {
        return FieldDescriptorType::Map;
    }
    if (starts_with(normalized, "ref<"))
    {
        return FieldDescriptorType::Reference;
    }
    return FieldDescriptorType::Named;
}

std::string field_descriptor_type_name(FieldDescriptorType type)
{
    switch (type)
    {
    case FieldDescriptorType::String:
        return "string";
    case FieldDescriptorType::Bool:
        return "bool";
    case FieldDescriptorType::Int:
        return "int";
    case FieldDescriptorType::Int32:
        return "int32";
    case FieldDescriptorType::Int64:
        return "int64";
    case FieldDescriptorType::Long:
        return "long";
    case FieldDescriptorType::Double:
        return "double";
    case FieldDescriptorType::Decimal:
        return "decimal";
    case FieldDescriptorType::Json:
        return "json";
    case FieldDescriptorType::Timestamp:
        return "timestamp";
    case FieldDescriptorType::Duration:
        return "duration";
    case FieldDescriptorType::Uuid:
        return "uuid";
    case FieldDescriptorType::Named:
        return "named";
    case FieldDescriptorType::List:
        return "list";
    case FieldDescriptorType::Set:
        return "set";
    case FieldDescriptorType::Map:
        return "map";
    case FieldDescriptorType::Optional:
        return "optional";
    case FieldDescriptorType::Reference:
        return "reference";
    }
    return "named";
}

GenerationResult generate_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    if (!validate_binding_generation_request(spec, options, diagnostics))
    {
        return GenerationResult{};
    }

    const auto system = lower_to_ir(spec);

    switch (options.language)
    {
    case BindingLanguage::Cpp:
        return filter_generation_result_by_tier(
            generate_cpp_bindings(system, options, diagnostics), options.tier
        );
    case BindingLanguage::Go:
        return filter_generation_result_by_tier(
            generate_go_bindings(system, options, diagnostics), options.tier
        );
    case BindingLanguage::Java:
        return filter_generation_result_by_tier(
            generate_java_bindings(system, options, diagnostics), options.tier
        );
    case BindingLanguage::Rust:
        return filter_generation_result_by_tier(
            generate_rust_bindings(system, options, diagnostics), options.tier
        );
    }

    diagnostics.error(SourceRange{}, "SSPEC5104", "unsupported binding generator language");
    return GenerationResult{};
}

} // namespace statespec
