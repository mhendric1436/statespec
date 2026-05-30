#include "generator_java_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_entity_index_helpers.hpp"
#include "generator_java_artifact_makefile.hpp"
#include "generator_java_descriptor_areas.hpp"
#include "generator_java_descriptor_support.hpp"
#include "generator_java_descriptors.hpp"
#include "generator_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace statespec
{
namespace
{

std::string java_api_default_handler_shape_import(const IrSystem& system);
std::string java_api_shape_import(const IrSystem& system);
std::filesystem::path java_api_generated_path(std::string_view filename);
void add_java_raw_api_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const std::filesystem::path& relative_output_path,
    std::string content
);
std::string java_workflow_worker_module_class_name(const IrWorkflow& workflow);
std::filesystem::path java_worker_generated_path(std::string_view filename);

std::string java_workflow_descriptor_module_class_name(std::string_view workflow_name)
{
    return pascal_identifier(std::string{workflow_name}) + "DescriptorModule";
}

std::string java_worker_descriptor_module_class_name(std::string_view worker_name)
{
    return pascal_identifier(std::string{worker_name}) + "DescriptorModule";
}

std::string java_shape_descriptor_module_class_name(std::string_view shape_name)
{
    return pascal_identifier(std::string{shape_name}) + "DescriptorModule";
}

std::string replace_all_copy(
    std::string value,
    std::string_view from,
    std::string_view to
)
{
    std::size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos)
    {
        value.replace(pos, from.size(), to);
        pos += to.size();
    }
    return value;
}

std::string java_event_descriptor_module_file(const IrSystem& system)
{
    std::ostringstream out;
    out << "package com.statespec.generated.descriptors;\n\n";
    out << "import com.statespec.backend.Json;\n";
    out << "import com.statespec.generated.descriptors.types.EventDescriptor;\n";
    out << "import com.statespec.generated.descriptors.types.EventEnvelope;\n";
    out << "import com.statespec.backend.Backend.FieldDescriptor;\n";
    out << "import com.statespec.backend.Backend.FieldType;\n";
    out << "import java.util.List;\n";
    out << "import java.util.Map;\n\n";
    out << "public final class EventDescriptorModule {\n";
    out << "    private EventDescriptorModule() {}\n\n";
    out << generate_java_event_descriptors(system);
    for (const auto& event : system.events)
    {
        out << "    public static EventEnvelope build" << pascal_identifier(event.name)
            << "Event(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "        Json " << lower_camel_identifier(field.name);
            out << (i + 1 < event.fields.size() ? "," : "") << "\n";
        }
        out << "    ) {\n";
        out << "        return new EventEnvelope(\n";
        out << "            " << java_string(event.name) << ",\n";
        out << "            Map.of(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "                " << java_string(field.name) << ", "
                << lower_camel_identifier(field.name);
            out << (i + 1 < event.fields.size() ? "," : "") << "\n";
        }
        out << "            )\n";
        out << "        );\n";
        out << "    }\n\n";
    }
    out << "}\n";
    return out.str();
}

std::vector<std::pair<
    std::string,
    std::string>>
java_descriptor_type_files()
{
    return generate_java_descriptor_type_files();
}

std::string java_worker_registry_module_file(
    const IrWorker& worker,
    std::string_view package_name,
    std::string_view class_name
)
{
    std::ostringstream out;
    out << "package " << package_name << ";\n\n";
    out << "import com.statespec.generated.descriptors.types.WorkerContext;\n";
    out << "import com.statespec.generated.descriptors.types.WorkerDescriptor;\n";
    out << "import com.statespec.generated.worker.descriptors."
        << java_worker_descriptor_module_class_name(worker.name) << ";\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class " << class_name << " {\n";
    out << "    private " << class_name << "() {}\n\n";
    out << "    public static Optional<WorkerDescriptor> findWorkerDescriptor(String "
           "workerName) {\n";
    out << "        if (" << java_string(worker.name) << ".equals(workerName)) {\n";
    out << "            return Optional.of("
        << java_worker_descriptor_module_class_name(worker.name) << ".workerDescriptor());\n";
    out << "        }\n";
    out << "        return Optional.empty();\n";
    out << "    }\n\n";
    out << "    public static Optional<WorkerContext> findWorkerContext(String "
           "workerName) {\n";
    out << "        if (" << java_string(worker.name) << ".equals(workerName)) {\n";
    out << "            return Optional.of("
        << java_worker_descriptor_module_class_name(worker.name) << ".workerContext());\n";
    out << "        }\n";
    out << "        return Optional.empty();\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string java_worker_registry_facade_file(const IrSystem& system)
{
    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << "import com.statespec.generated.descriptors.types.WorkerContext;\n";
    out << "import com.statespec.generated.descriptors.types.WorkerDescriptor;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class WorkerRegistry {\n";
    out << "    private WorkerRegistry() {}\n\n";
    out << "    public static Optional<WorkerDescriptor> findWorkerDescriptor(String "
           "workerName) {\n";
    for (const auto& worker : system.workers)
    {
        out << "        var " << lower_camel_identifier(worker.name)
            << "Worker = com.statespec.generated.registry." << pascal_identifier(worker.name)
            << "Registry.findWorkerDescriptor(workerName);\n";
        out << "        if (" << lower_camel_identifier(worker.name) << "Worker.isPresent()) {\n";
        out << "            return " << lower_camel_identifier(worker.name) << "Worker;\n";
        out << "        }\n";
    }
    out << "        return Optional.empty();\n";
    out << "    }\n\n";
    out << "    public static Optional<WorkerContext> findWorkerContext(String "
           "workerName) {\n";
    for (const auto& worker : system.workers)
    {
        out << "        var " << lower_camel_identifier(worker.name)
            << "Context = com.statespec.generated.registry." << pascal_identifier(worker.name)
            << "Registry.findWorkerContext(workerName);\n";
        out << "        if (" << lower_camel_identifier(worker.name) << "Context.isPresent()) {\n";
        out << "            return " << lower_camel_identifier(worker.name) << "Context;\n";
        out << "        }\n";
    }
    out << "        return Optional.empty();\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string java_external_system_descriptor_module_file(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    std::ostringstream out;
    out << "package com.statespec.generated.descriptors;\n\n";
    out << "import com.statespec.generated.descriptors.types.*;\n";
    out << "import java.util.List;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class ExternalSystemDescriptorModule {\n";
    out << "    private ExternalSystemDescriptorModule() {}\n\n";
    (void)templates;
    out << generate_java_external_system_descriptors(system);
    out << "}\n";
    return out.str();
}

std::vector<std::pair<
    std::string,
    std::string>>
java_runtime_registration_modules(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::vector<std::pair<std::string, std::string>> modules;
    auto add = [&](bool used, std::string name, std::string class_name)
    {
        if (used)
        {
            modules.emplace_back(std::move(name), std::move(class_name));
        }
    };
    add(usage.uses_feature_flags, "feature_flags", "RuntimeFeatureFlagRegistrationModule");
    add(usage.uses_queues, "queues", "RuntimeQueueRegistrationModule");
    add(usage.uses_leases, "leases", "RuntimeLeaseRegistrationModule");
    add(usage.uses_logs, "logs", "RuntimeLogRegistrationModule");
    add(usage.uses_metrics, "metrics", "RuntimeMetricRegistrationModule");
    add(usage.uses_logs && usage.uses_metrics, "observability",
        "RuntimeObservabilityRegistrationModule");
    add(usage.uses_workflows, "workflows", "RuntimeWorkflowRegistrationModule");
    return modules;
}

std::string java_runtime_registration_module_file(
    const TemplatePackage& templates,
    std::string_view name,
    std::string_view class_name
)
{
    std::ostringstream out;
    out << "package com.statespec.generated.descriptors;\n\n";
    out << "import com.statespec.backend.Backend;\n";
    out << "import com.statespec.backend.FeatureFlag;\n";
    out << "import com.statespec.backend.Lease;\n";
    out << "import com.statespec.backend.Log;\n";
    out << "import com.statespec.backend.Metric;\n";
    out << "import com.statespec.backend.Queue;\n";
    out << "import com.statespec.backend.Workflow;\n";
    out << "import com.statespec.generated.descriptors.types.*;\n";
    out << "\n";
    out << "import static com.statespec.generated.Descriptors.*;\n\n";
    out << "public final class " << class_name << " {\n";
    out << "    private " << class_name << "() {}\n\n";
    out << generate_java_runtime_registration_domain(templates, name);
    out << "}\n";
    return out.str();
}

std::string java_api_descriptor_module_class_name(std::string_view api_name)
{
    return pascal_identifier(std::string{api_name}) + "DescriptorModule";
}

struct ApiHandlerDomain
{
    std::string name;
    std::vector<IrApi> apis;
};

bool api_output_mentions_entity(
    const IrApi& api,
    const IrEntity& entity
)
{
    return api.output.has_value() && api.output->find(entity.name) != std::string::npos;
}

bool api_input_mentions_entity(
    const IrApi& api,
    const IrEntity& entity
)
{
    return api.input.has_value() && api.input->find(entity.name) != std::string::npos;
}

bool api_name_mentions_entity(
    const IrApi& api,
    const IrEntity& entity
)
{
    return api.name.find(entity.name) != std::string::npos;
}

std::vector<ApiHandlerDomain> api_handler_domains(const IrSystem& system)
{
    std::vector<ApiHandlerDomain> domains;
    for (const auto& entity : system.entities)
    {
        domains.push_back(ApiHandlerDomain{entity.name, {}});
    }
    ApiHandlerDomain operations{"Operations", {}};
    for (const auto& api : system.apis)
    {
        bool assigned = false;
        auto assign_matching_entity = [&](auto matches)
        {
            for (std::size_t i = 0; i < system.entities.size(); ++i)
            {
                if (matches(api, system.entities[i]))
                {
                    domains[i].apis.push_back(api);
                    assigned = true;
                    return;
                }
            }
        };
        assign_matching_entity(api_output_mentions_entity);
        if (!assigned)
        {
            assign_matching_entity(api_input_mentions_entity);
        }
        if (!assigned)
        {
            assign_matching_entity(api_name_mentions_entity);
        }
        if (!assigned)
        {
            operations.apis.push_back(api);
        }
    }
    domains.erase(
        std::remove_if(
            domains.begin(), domains.end(), [](const auto& domain) { return domain.apis.empty(); }
        ),
        domains.end()
    );
    if (!operations.apis.empty())
    {
        domains.push_back(std::move(operations));
    }
    return domains;
}

IrSystem with_domain_apis(
    const IrSystem& system,
    const std::vector<IrApi>& apis
)
{
    auto filtered = system;
    filtered.apis = apis;
    return filtered;
}

std::filesystem::path java_api_handler_domain_path(std::string_view domain_name)
{
    return java_api_generated_path("entities") / snake_identifier(std::string{domain_name}) /
           "Handlers.java";
}

std::filesystem::path java_api_handler_domain_registry_path(std::string_view domain_name)
{
    return java_api_generated_path("entities") / snake_identifier(std::string{domain_name}) /
           "Registry.java";
}

std::filesystem::path java_entity_api_catalog_path(std::string_view entity_name)
{
    return java_api_generated_path("entities") / snake_identifier(std::string{entity_name}) /
           "Catalog.java";
}

std::string java_api_handler_registry_domain_imports(const std::vector<ApiHandlerDomain>& domains)
{
    (void)domains;
    std::ostringstream out;
    return out.str();
}

std::string java_api_handler_lookup_registrations(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        out << "        com.statespec.generated.entities." << snake_identifier(domain.name)
            << ".Registry.registerHandlerInvokers(handlers);\n";
    }
    return out.str();
}

bool is_entity_crud_api_java(const IrApi& api)
{
    return api.entity.has_value() && api.repository_operation.has_value();
}

std::vector<ApiHandlerDomain>
crud_api_handler_domains_java(const std::vector<ApiHandlerDomain>& domains)
{
    std::vector<ApiHandlerDomain> result;
    for (const auto& domain : domains)
    {
        for (const auto& api : domain.apis)
        {
            if (is_entity_crud_api_java(api))
            {
                auto target = std::find_if(
                    result.begin(), result.end(),
                    [&](const auto& candidate) { return candidate.name == *api.entity; }
                );
                if (target == result.end())
                {
                    result.push_back(ApiHandlerDomain{*api.entity, {api}});
                }
                else
                {
                    target->apis.push_back(api);
                }
            }
        }
    }
    return result;
}

std::string java_business_api_handler_lookup_entries(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        if (is_entity_crud_api_java(api))
        {
            continue;
        }
        out << "        handlers.put(" << java_string(api.name) << ", "
            << "ApiHandlers.BusinessHandler::handle" << pascal_identifier(api.name) << ");\n";
    }
    return out.str();
}

std::string java_api_handler_domain_file(
    const IrSystem& system,
    const ApiHandlerDomain& domain
)
{
    const auto filtered = with_domain_apis(system, domain.apis);
    std::ostringstream out;
    out << "package com.statespec.generated.entities." << snake_identifier(domain.name) << ";\n\n";
    out << "import static com.statespec.generated.ApiHandlerRegistry.*;\n";
    out << "import com.statespec.generated.ApiCodecs;\n";
    out << "import com.statespec.generated.ApiRequestContext;\n";
    out << "import com.statespec.generated.ApiResponse;\n";
    out << java_api_default_handler_shape_import(filtered);
    out << "import java.util.Optional;\n\n";
    out << "public final class Handlers {\n";
    out << "    private Handlers() {}\n\n";
    out << "    public static final class DefaultHandler {\n";
    out << "        private final com.statespec.backend.Backend backend;\n\n";
    out << "        public DefaultHandler(com.statespec.backend.Backend backend) {\n";
    out << "            this.backend = backend;\n";
    out << "        }\n\n";
    out << generate_api_operation_default_handler_domain_methods_java(filtered);
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string java_api_handler_domain_registry_file(const ApiHandlerDomain& domain)
{
    std::ostringstream out;
    out << "package com.statespec.generated.entities." << snake_identifier(domain.name) << ";\n\n";
    out << "import com.statespec.generated.ApiDispatcher;\n";
    out << "import com.statespec.generated.ApiRequestContext;\n";
    out << "import com.statespec.generated.ApiResponse;\n\n";
    out << "import java.util.Map;\n\n";
    out << "public final class Registry {\n";
    out << "    private Registry() {}\n\n";
    for (const auto& api : domain.apis)
    {
        out << "    public static ApiResponse handle" << pascal_identifier(api.name)
            << "(com.statespec.backend.Backend backend, ApiRequestContext context) "
               "throws Exception {\n";
        out << "        return new Handlers.DefaultHandler(backend).handle"
            << pascal_identifier(api.name) << "(context);\n";
        out << "    }\n\n";
    }
    out << "    public static void registerHandlerInvokers(\n";
    out << "        Map<String, ApiDispatcher.HandlerInvoker> handlers\n";
    out << "    ) {\n";
    for (const auto& api : domain.apis)
    {
        out << "        handlers.put(" << java_string(api.name) << ", Registry::handle"
            << pascal_identifier(api.name) << ");\n";
    }
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::vector<IrShape> api_codec_shapes(const IrSystem& system)
{
    std::vector<IrShape> shapes;
    for (const auto& shape : system.shapes)
    {
        const auto used = std::any_of(
            system.apis.begin(), system.apis.end(),
            [&](const auto& api)
            {
                return (api.input.has_value() && *api.input == shape.name) ||
                       (api.output.has_value() && *api.output == shape.name);
            }
        );
        if (used)
        {
            shapes.push_back(shape);
        }
    }
    return shapes;
}

std::optional<std::string> java_entity_api_shape_owner(
    const IrSystem& system,
    std::string_view shape_name
)
{
    for (const auto& api : system.apis)
    {
        if (!api.entity.has_value() || !api.repository_operation.has_value())
        {
            continue;
        }
        if ((api.input.has_value() && *api.input == shape_name) ||
            (api.output.has_value() && *api.output == shape_name))
        {
            return *api.entity;
        }
    }
    return std::nullopt;
}

std::vector<IrShape> java_entity_api_shapes(
    const IrSystem& system,
    std::string_view entity_name
)
{
    std::vector<IrShape> shapes;
    for (const auto& shape : system.shapes)
    {
        const auto owner = java_entity_api_shape_owner(system, shape.name);
        if (owner.has_value() && *owner == entity_name)
        {
            shapes.push_back(shape);
        }
    }
    return shapes;
}

IrSystem with_codec_shape_apis(
    const IrSystem& system,
    std::string_view shape_name
)
{
    auto filtered = system;
    filtered.apis.clear();
    for (const auto& api : system.apis)
    {
        IrApi scoped = api;
        if (!scoped.input.has_value() || *scoped.input != shape_name)
        {
            scoped.input.reset();
        }
        if (!scoped.output.has_value() || *scoped.output != shape_name)
        {
            scoped.output.reset();
        }
        if (scoped.input.has_value() || scoped.output.has_value())
        {
            filtered.apis.push_back(std::move(scoped));
        }
    }
    return filtered;
}

std::string java_api_codec_shape_class_name(std::string_view shape_name)
{
    return "ApiCodecs" + pascal_identifier(std::string{shape_name});
}

std::filesystem::path java_api_codec_shape_path(std::string_view shape_name)
{
    return std::filesystem::path{"api/com/statespec/generated/codecs"} /
           (java_api_codec_shape_class_name(shape_name) + ".java");
}

std::filesystem::path java_entity_api_codec_path(std::string_view entity_name)
{
    return java_api_generated_path("entities") / snake_identifier(std::string{entity_name}) /
           "Codecs.java";
}

bool java_api_has_non_entity_codec_shapes(const IrSystem& system)
{
    for (const auto& shape : api_codec_shapes(system))
    {
        if (!java_entity_api_shape_owner(system, shape.name).has_value())
        {
            return true;
        }
    }
    return false;
}

std::string java_api_codec_imports(const IrSystem& system)
{
    return java_api_has_non_entity_codec_shapes(system)
               ? "import com.statespec.generated.codecs.*;\n"
               : "";
}

IrSystem with_codec_shapes_apis(
    const IrSystem& system,
    const std::vector<IrShape>& shapes
)
{
    auto filtered = system;
    filtered.apis.clear();
    std::set<std::string> shape_names;
    for (const auto& shape : shapes)
    {
        shape_names.insert(shape.name);
    }
    for (const auto& api : system.apis)
    {
        IrApi scoped = api;
        if (!scoped.input.has_value() || shape_names.count(*scoped.input) == 0)
        {
            scoped.input.reset();
        }
        if (!scoped.output.has_value() || shape_names.count(*scoped.output) == 0)
        {
            scoped.output.reset();
        }
        if (scoped.input.has_value() || scoped.output.has_value())
        {
            filtered.apis.push_back(std::move(scoped));
        }
    }
    return filtered;
}

std::string java_api_codec_shape_file(
    const IrSystem& system,
    const IrShape& shape
)
{
    const auto filtered = with_codec_shape_apis(system, shape.name);
    std::ostringstream out;
    out << "package com.statespec.generated.codecs;\n\n";
    out << "import static com.statespec.generated.ApiCodecs.*;\n";
    out << "import com.statespec.backend.Json;\n";
    out << "import com.statespec.generated.ApiRequestContext;\n";
    out << "import com.statespec.generated.ApiResponse;\n";
    out << java_api_shape_import(filtered);
    out << "import java.util.Map;\n";
    out << "import java.util.Optional;\n";
    out << "import java.util.TreeMap;\n\n";
    out << "public final class " << java_api_codec_shape_class_name(shape.name) << " {\n";
    out << "    private " << java_api_codec_shape_class_name(shape.name) << "() {}\n\n";
    out << generate_api_codec_operations_java(filtered);
    out << "}\n";
    return out.str();
}

std::string java_entity_api_codec_file(
    const IrSystem& system,
    const IrEntity& entity
)
{
    const auto shapes = java_entity_api_shapes(system, entity.name);
    const auto filtered = with_codec_shapes_apis(system, shapes);
    std::ostringstream out;
    out << "package com.statespec.generated.entities." << snake_identifier(entity.name) << ";\n\n";
    out << "import static com.statespec.generated.ApiCodecs.*;\n";
    out << "import static com.statespec.generated.entities." << snake_identifier(entity.name)
        << ".Shapes.*;\n";
    out << "import com.statespec.backend.Json;\n";
    out << "import com.statespec.generated.ApiRequestContext;\n";
    out << "import com.statespec.generated.ApiResponse;\n";
    out << "import com.statespec.generated.entities." << snake_identifier(entity.name)
        << ".Constants;\n";
    out << "import java.util.Map;\n";
    out << "import java.util.Optional;\n";
    out << "import java.util.TreeMap;\n\n";
    out << "public final class Codecs {\n";
    out << "    private Codecs() {}\n\n";
    out << generate_api_codec_operations_java(filtered);
    out << "}\n";
    return out.str();
}

std::string java_api_codec_delegates(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& shape : api_codec_shapes(system))
    {
        const auto filtered = with_codec_shape_apis(system, shape.name);
        const auto owner = java_entity_api_shape_owner(system, shape.name);
        const auto class_name = owner.has_value() ? "com.statespec.generated.entities." +
                                                        snake_identifier(*owner) + ".Codecs"
                                                  : java_api_codec_shape_class_name(shape.name);
        for (const auto& api : filtered.apis)
        {
            if (api.input.has_value())
            {
                out << "    public static " << pascal_identifier(*api.input) << " decode"
                    << pascal_identifier(api.name) << "Request(ApiRequestContext request) {\n";
                out << "        return " << class_name << ".decode" << pascal_identifier(api.name)
                    << "Request(request);\n";
                out << "    }\n\n";
            }
            if (api.output.has_value())
            {
                out << "    public static " << pascal_identifier(*api.output) << " decode"
                    << pascal_identifier(api.name) << "Response(ApiResponse response) {\n";
                out << "        return " << class_name << ".decode" << pascal_identifier(api.name)
                    << "Response(response);\n";
                out << "    }\n\n";
                out << "    public static ApiResponse encode" << pascal_identifier(api.name)
                    << "Response(" << pascal_identifier(*api.output) << " response) {\n";
                out << "        return " << class_name << ".encode" << pascal_identifier(api.name)
                    << "Response(response);\n";
                out << "    }\n\n";
                out << "    public static ApiResponse encode" << pascal_identifier(api.name)
                    << "Response(" << pascal_identifier(*api.output)
                    << " response, int statusCode) {\n";
                out << "        return " << class_name << ".encode" << pascal_identifier(api.name)
                    << "Response(response, statusCode);\n";
                out << "    }\n\n";
            }
        }
    }
    return out.str();
}

TemplateRenderer::Values java_runtime_bootstrap_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream imports;
    std::ostringstream fields;
    std::ostringstream initializers;
    std::ostringstream arguments;
    std::ostringstream worker_imports;
    std::ostringstream worker_run_once;

    auto add = [&](bool used, std::string_view type, std::string_view field)
    {
        if (!used)
        {
            return;
        }
        imports << "import com.statespec.backend.runtime." << type << ";\n";
        fields << "    private final " << type << " " << field << ";\n";
        initializers << "        this." << field << " = new " << type << "();\n";
        arguments << ", " << field;
    };
    add(usage.uses_feature_flags, "FeatureFlagStore", "featureFlags");
    add(usage.uses_queues, "QueueStore", "queues");
    add(usage.uses_leases, "LeaseStore", "leases");
    add(usage.uses_workflows, "WorkflowStore", "workflows");
    add(usage.uses_logs, "LogSink", "logs");
    add(usage.uses_metrics, "MetricSink", "metrics");
    if (usage.uses_workflows)
    {
        worker_imports << "import java.time.Duration;\n";
        worker_imports << "import java.util.Optional;\n";
        worker_run_once
            << "    public Optional<com.statespec.backend.Workflow.WorkflowExecutionRecord> "
               "runOnce(\n"
            << "        com.statespec.generated.descriptors.types.WorkerContext context,\n"
            << "        WorkflowStepHandlers.Handler handler,\n"
            << "        String workflowExecutionId\n"
            << "    ) throws Exception {\n"
            << "        if (context.executes().isEmpty()) {\n"
            << "            return Optional.empty();\n"
            << "        }\n"
            << "        WorkflowRunner runner =\n"
            << "            new WorkflowRunner(backend, workflows, handler, context.workerName(), "
               "Duration.ofSeconds(30), 3);\n"
            << "        return runner.runOnce(workflowExecutionId, "
               "context.executes().orElseThrow(), "
               "1);\n"
            << "    }\n";
    }
    else
    {
        worker_imports << "import java.util.Optional;\n";
        worker_run_once
            << "    public Optional<com.statespec.backend.Workflow.WorkflowExecutionRecord> "
               "runOnce(\n"
            << "        com.statespec.generated.descriptors.types.WorkerContext context,\n"
            << "        WorkflowStepHandlers.Handler handler,\n"
            << "        String workflowExecutionId\n"
            << "    ) {\n"
            << "        return Optional.empty();\n"
            << "    }\n";
    }
    return TemplateRenderer::Values{
        {"runtime_store_imports", imports.str()},
        {"runtime_store_fields", fields.str()},
        {"runtime_store_initializers", initializers.str()},
        {"runtime_bootstrap_arguments", arguments.str()},
        {"worker_runtime_imports", worker_imports.str()},
        {"worker_runtime_run_once", worker_run_once.str()},
    };
}

TemplateRenderer::Values java_api_runtime_bootstrap_values(const IrSystem& system)
{
    auto values = java_runtime_bootstrap_values(system);
    values.erase("worker_runtime_imports");
    values.erase("worker_runtime_run_once");
    return values;
}

std::string java_workflow_worker_module_class_name(const IrWorkflow& workflow)
{
    return pascal_identifier(workflow.name) + "WorkerModule";
}

std::string generate_java_workflow_worker_module(const IrWorkflow& workflow)
{
    const auto class_name = java_workflow_worker_module_class_name(workflow);
    const auto handler_package = snake_identifier(workflow.name);
    std::ostringstream out;
    out << "package com.statespec.generated.workflows;\n\n";
    out << "import com.statespec.generated.WorkflowStepHandlers;\n";
    out << "import com.statespec.generated.workflows." << handler_package << ".Handlers;\n";
    out << "import com.statespec.backend.Workflow;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class " << class_name << " {\n";
    out << "    private " << class_name << "() {}\n\n";
    out << "    public static boolean dispatchStep(\n";
    out << "        Handlers." << pascal_identifier(workflow.name) << "V"
        << workflow.version.value_or(1) << "StepHandler handler,\n";
    out << "        WorkflowStepHandlers.Context context,\n";
    out << "        Workflow.WorkflowExecutionRecord record\n";
    out << "    ) throws Exception {\n";
    out << "        if (!record.workflowName().equals(" << java_string(workflow.name) << ")) {\n";
    out << "            return false;\n";
    out << "        }\n";
    for (const auto& step : workflow.steps)
    {
        out << "        if (record.currentStep().equals(" << java_string(step.name) << ")) {\n";
        out << "            handler.handle" << pascal_identifier(step.name) << "(context);\n";
        out << "            return true;\n";
        out << "        }\n";
    }
    out << "        return false;\n";
    out << "    }\n\n";
    out << "    public static Optional<String> nextStep(Workflow.WorkflowExecutionRecord record) "
           "{\n";
    out << "        if (!record.workflowName().equals(" << java_string(workflow.name) << ")) {\n";
    out << "            return Optional.empty();\n";
    out << "        }\n";
    for (const auto& step : workflow.steps)
    {
        for (const auto& statement : step.statements)
        {
            if (statement.kind != "transition_to" || !statement.target.has_value())
            {
                continue;
            }
            out << "        if (record.currentStep().equals(" << java_string(step.name) << ")) {\n";
            out << "            return Optional.of(" << java_string(*statement.target) << ");\n";
            out << "        }\n";
        }
    }
    out << "        return Optional.empty();\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string generate_java_workflow_handler_module(const IrWorkflow& workflow)
{
    const auto class_name = pascal_identifier(workflow.name) + "V" +
                            std::to_string(workflow.version.value_or(1)) + "StepHandler";
    std::ostringstream out;
    out << "package com.statespec.generated.workflows." << snake_identifier(workflow.name)
        << ";\n\n";
    out << "import com.statespec.generated.WorkflowStepHandlers;\n\n";
    out << "public final class Handlers {\n";
    out << "    private Handlers() {}\n\n";
    out << "    public interface " << class_name << " {\n";
    for (const auto& step : workflow.steps)
    {
        out << "        void handle" << pascal_identifier(step.name)
            << "(WorkflowStepHandlers.Context context) throws Exception;\n";
    }
    out << "    }\n\n";
    out << "    public static final class Default" << class_name << " implements " << class_name
        << " {\n";
    for (const auto& step : workflow.steps)
    {
        out << "        @Override\n";
        out << "        public void handle" << pascal_identifier(step.name)
            << "(WorkflowStepHandlers.Context context) {\n";
        out << "            throw new UnsupportedOperationException(\"generated workflow step "
               "handler "
            << workflow.name << "." << step.name << " is not implemented\");\n";
        out << "        }\n";
    }
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string java_workflow_step_handler_imports(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        out << "import com.statespec.generated.workflows." << snake_identifier(workflow.name)
            << ".Handlers;\n";
    }
    return out.str();
}

std::string java_workflow_step_handler_bases(const IrSystem& system)
{
    std::ostringstream out;
    bool first = true;
    for (const auto& workflow : system.workflows)
    {
        out << (first ? " extends " : ", ") << "Handlers." << pascal_identifier(workflow.name)
            << "V" << workflow.version.value_or(1) << "StepHandler";
        first = false;
    }
    return out.str();
}

TemplateRenderer::Values java_workflow_runner_values(const IrSystem& system)
{
    std::ostringstream imports;
    std::ostringstream dispatch_cases;
    std::ostringstream next_cases;
    for (const auto& workflow : system.workflows)
    {
        const auto class_name = java_workflow_worker_module_class_name(workflow);
        imports << "import com.statespec.generated.workflows." << class_name << ";\n";
        dispatch_cases << "            if (!handled) {\n";
        dispatch_cases << "                handled = " << class_name
                       << ".dispatchStep(handler, context, record);\n";
        dispatch_cases << "            }\n";
        next_cases << "            if (nextStep.isEmpty()) {\n";
        next_cases << "                nextStep = " << class_name << ".nextStep(record);\n";
        next_cases << "            }\n";
    }
    return TemplateRenderer::Values{
        {"workflow_step_module_imports", imports.str()},
        {"workflow_step_module_dispatch_cases", dispatch_cases.str()},
        {"workflow_step_module_next_cases", next_cases.str()},
    };
}

std::string java_entity_gc_descriptor_file(const IrEntity& entity)
{
    std::ostringstream terminal_states;
    bool first_terminal_state = true;
    for (const auto& state : entity.states)
    {
        if (!state.garbage_collection.has_value())
        {
            continue;
        }
        if (!first_terminal_state)
        {
            terminal_states << ",\n";
        }
        terminal_states << "                new EntityGcTypes.TerminalState("
                        << "Constants." << java_entity_state_constant_name(entity.name, state.name)
                        << ", Schema."
                        << upper_snake_identifier(
                               entity.name + "_state_" + state.name + "_gc_after"
                           )
                        << ", Schema."
                        << upper_snake_identifier(entity.name + "_state_" + state.name + "_gc_mode")
                        << ")";
        first_terminal_state = false;
    }

    const auto package_name = "com.statespec.generated.entities." + snake_identifier(entity.name);
    std::ostringstream out;
    out << "package " << package_name << ";\n\n";
    out << "import com.statespec.backend.runtime.EntityGcTypes;\n";
    out << "import java.util.List;\n\n";
    out << "public final class Gc\n";
    out << "{\n";
    out << "    private Gc() {}\n\n";
    out << "    public static EntityGcTypes.Descriptor descriptor()\n";
    out << "    {\n";
    out << "        return new EntityGcTypes.Descriptor(\n";
    out << "            Constants." << java_entity_name_constant_name(entity.name) << ",\n";
    out << "            Constants." << java_entity_name_constant_name(entity.name) << ",\n";
    out << "            Constants." << java_entity_field_constant_name(entity.name, "status")
        << ",\n";
    out << "            List.of(\n";
    out << terminal_states.str() << "\n";
    out << "            )\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string java_api_entity_gc_catalog_file(const IrSystem& system)
{
    std::vector<std::string> descriptor_calls;
    for (const auto& entity : system.entities)
    {
        const auto entity_uses_gc = std::any_of(
            entity.states.begin(), entity.states.end(),
            [](const IrState& state) { return state.garbage_collection.has_value(); }
        );
        if (!entity_uses_gc)
        {
            continue;
        }
        descriptor_calls.push_back(
            "com.statespec.generated.entities." + snake_identifier(entity.name) + ".Gc.descriptor()"
        );
    }

    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << "import com.statespec.backend.runtime.EntityGcTypes;\n";
    out << "import java.util.List;\n\n";
    out << "public final class EntityGcCatalog {\n";
    out << "    private EntityGcCatalog() {}\n\n";
    out << "    public static List<EntityGcTypes.Descriptor> descriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t i = 0; i < descriptor_calls.size(); ++i)
    {
        out << "            " << descriptor_calls[i];
        if (i + 1 < descriptor_calls.size())
        {
            out << ",";
        }
        out << "\n";
    }
    out << "        );\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string java_worker_entity_gc_catalog_file(const IrSystem& system)
{
    std::vector<std::string> descriptor_calls;
    for (const auto& entity : system.entities)
    {
        const auto entity_uses_gc = std::any_of(
            entity.states.begin(), entity.states.end(),
            [](const IrState& state) { return state.garbage_collection.has_value(); }
        );
        if (!entity_uses_gc)
        {
            continue;
        }
        descriptor_calls.push_back(
            "com.statespec.generated.entities." + snake_identifier(entity.name) + ".Gc.descriptor()"
        );
    }

    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << "import com.statespec.backend.runtime.EntityGcTypes;\n";
    out << "import java.util.List;\n\n";
    out << "public final class WorkerEntityGcCatalog {\n";
    out << "    private WorkerEntityGcCatalog() {}\n\n";
    out << "    public static List<EntityGcTypes.Descriptor> descriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t i = 0; i < descriptor_calls.size(); ++i)
    {
        out << "            " << descriptor_calls[i];
        if (i + 1 < descriptor_calls.size())
        {
            out << ",";
        }
        out << "\n";
    }
    out << "        );\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

TemplateRenderer::Values java_api_main_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    if (!usage.uses_entity_gc)
    {
        return TemplateRenderer::Values{
            {"api_main_entity_gc_import", ""},
            {"api_main_entity_gc_registration", ""},
        };
    }
    return TemplateRenderer::Values{
        {"api_main_entity_gc_import",
         "import com.statespec.backend.runtime.EntityGcRegistration;\n"},
        {"api_main_entity_gc_registration",
         "            EntityGcRegistration.registerEntityGcWorkers(\n"
         "                task -> process.addEntityGcWorker(task::run), backend, "
         "EntityGcCatalog.descriptors()\n"
         "            );\n\n"},
    };
}

TemplateRenderer::Values java_worker_main_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    if (!usage.uses_entity_gc)
    {
        return TemplateRenderer::Values{
            {"worker_main_entity_gc_import", ""},
            {"worker_main_entity_gc_registration", ""},
        };
    }
    return TemplateRenderer::Values{
        {"worker_main_entity_gc_import",
         "import com.statespec.backend.runtime.EntityGcRegistration;\n"},
        {"worker_main_entity_gc_registration",
         "            EntityGcRegistration.registerEntityGcWorkers(\n"
         "                task -> runtime.addEntityGcWorker(task::run), backend, "
         "WorkerEntityGcCatalog.descriptors()\n"
         "            );\n\n"},
    };
}

std::filesystem::path java_api_generated_path(std::string_view filename)
{
    return api_artifact_path(
        join_artifact_path(GeneratedJavaOutputPackagePath, filename).generic_string()
    );
}

std::filesystem::path java_worker_generated_path(std::string_view filename)
{
    return worker_artifact_path(
        join_artifact_path(GeneratedJavaOutputPackagePath, filename).generic_string()
    );
}

void add_java_generated_template_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const std::filesystem::path& relative_output_path,
    GeneratedArtifactTier tier,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
)
{
    add_generated_template_file(
        result, options.output_dir, templates, template_path_for_output(relative_output_path),
        relative_output_path, diagnostics, tier, values
    );
}

TemplateRenderer::Values java_descriptor_module_values(
    std::string_view package_name,
    std::string_view class_name,
    std::string_view module_name,
    std::string content = {}
)
{
    return TemplateRenderer::Values{
        {"descriptor_module_package", std::string{package_name}},
        {"descriptor_module_class", std::string{class_name}},
        {"descriptor_module_name", std::string{module_name}},
        {"descriptor_module_content", std::move(content)},
    };
}

std::string java_entity_centered_facade_file(
    const IrEntity& entity,
    std::string_view class_name,
    std::string_view area
)
{
    std::ostringstream out;
    out << "package com.statespec.generated.entities." << snake_identifier(entity.name) << ";\n\n";
    if (area == "model")
    {
        out << "import static com.statespec.generated.entities." << snake_identifier(entity.name)
            << ".Constants.*;\n";
        out << "import com.statespec.backend.Backend.FieldDescriptor;\n";
        out << "import com.statespec.backend.Backend.FieldType;\n";
        out << "import com.statespec.backend.Backend.IndexDescriptor;\n";
        out << "import java.util.List;\n\n";
        out << "/** Entity-centered model metadata for " << entity.name << ". */\n";
    }
    else if (area == "schema")
    {
        out << "import com.statespec.backend.Backend.CollectionDescriptor;\n";
        out << "import java.util.List;\n\n";
        out << "/** Entity-centered collection schema and compatibility metadata for "
            << entity.name << ". */\n";
    }
    else
    {
        out << "/** Entity-centered " << area
            << " facade. Implementation moves here in the next staged split. */\n";
    }
    out << "public final class " << class_name << " {\n";
    out << "    private " << class_name << "() {}\n";
    if (area == "model")
    {
        out << "\n";
        out << "    public static List<FieldDescriptor> " << lower_camel_identifier(entity.name)
            << "FieldDescriptors() {\n";
        out << "        return List.of(\n";
        for (std::size_t i = 0; i < entity.fields.size(); ++i)
        {
            const auto& field = entity.fields[i];
            out << "            " << java_entity_field_descriptor_expr(entity.name, field);
            out << (i + 1 < entity.fields.size() ? ",\n" : "\n");
        }
        out << "        );\n";
        out << "    }\n\n";
        out << "    public static List<IndexDescriptor> " << lower_camel_identifier(entity.name)
            << "IndexDescriptors() {\n";
        out << "        return List.of(\n";
        for (std::size_t i = 0; i < entity.indexes.size(); ++i)
        {
            const auto& index = entity.indexes[i];
            out << "            new IndexDescriptor(\n";
            out << "                " << java_entity_index_constant_name(entity.name, index.name)
                << ",\n";
            out << "                List.of(";
            for (std::size_t j = 0; j < index.fields.size(); ++j)
            {
                if (j > 0)
                {
                    out << ", ";
                }
                out << java_entity_field_constant_name(entity.name, index.fields[j]);
            }
            out << "),\n";
            out << "                " << (index.unique ? "true" : "false") << "\n";
            out << "            )" << (i + 1 < entity.indexes.size() ? "," : "") << "\n";
        }
        out << "        );\n";
        out << "    }\n";
    }
    else if (area == "schema")
    {
        out << "\n";
        const auto schema_version_constant =
            upper_snake_identifier(entity.name + "_schema_version");
        out << "    public static final long " << schema_version_constant << " = 1L;\n";
        if (entity.ownership.has_value())
        {
            out << "    public static final String "
                << upper_snake_identifier(entity.name + "_ownership_authority") << " = "
                << java_string(entity.ownership->authority) << ";\n";
            out << "    public static final String "
                << upper_snake_identifier(entity.name + "_ownership_system_of_record") << " = "
                << java_string(entity.ownership->system_of_record) << ";\n";
            out << "    public static final String "
                << upper_snake_identifier(entity.name + "_ownership_lifecycle") << " = "
                << java_string(entity.ownership->lifecycle) << ";\n";
        }
        for (const auto& state : entity.states)
        {
            out << "    public static final boolean "
                << upper_snake_identifier(entity.name + "_state_" + state.name + "_terminal")
                << " = " << (state.terminal ? "true" : "false") << ";\n";
            if (state.garbage_collection.has_value())
            {
                out << "    public static final String "
                    << upper_snake_identifier(entity.name + "_state_" + state.name + "_gc_after")
                    << " = " << java_string(state.garbage_collection->after) << ";\n";
                out << "    public static final String "
                    << upper_snake_identifier(entity.name + "_state_" + state.name + "_gc_mode")
                    << " = " << java_string(state.garbage_collection->mode) << ";\n";
            }
        }
        out << "\n";
        out << "    public static CollectionDescriptor " << lower_camel_identifier(entity.name)
            << "CollectionDescriptor() {\n";
        out << "        return new CollectionDescriptor(\n";
        out << "            Constants." << java_entity_name_constant_name(entity.name) << ",\n";
        out << "            Model." << lower_camel_identifier(entity.name)
            << "FieldDescriptors(),\n";
        out << "            List.of(";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << "Constants."
                << java_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "),\n";
        out << "            Model." << lower_camel_identifier(entity.name)
            << "IndexDescriptors(),\n";
        out << "            " << schema_version_constant << "\n";
        out << "        );\n";
        out << "    }\n";
    }
    else if (area == "persistence")
    {
        const auto type_name = pascal_identifier(entity.name);
        out << "\n";
        out << "    public static com.statespec.generated.entities.EntityLookup build" << type_name
            << "Lookup(\n";
        out << "        java.util.List<com.statespec.generated.entities.EntityKeyValue> "
               "keyValues\n";
        out << "    ) {\n";
        out << "        return new com.statespec.generated.entities.EntityLookup(\n";
        out << "            Constants." << java_entity_name_constant_name(entity.name) << ",\n";
        out << "            java.util.List.of(";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << "Constants."
                << java_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "),\n";
        out << "            java.util.List.copyOf(keyValues)\n";
        out << "        );\n";
        out << "    }\n\n";
        out << "    public static com.statespec.generated.descriptors.types.EntityDescriptor "
            << lower_camel_identifier(entity.name) << "EntityDescriptor() {\n";
        out << "        return new com.statespec.generated.descriptors.types.EntityDescriptor(\n";
        out << "            Constants." << java_entity_name_constant_name(entity.name) << ",\n";
        out << "            java.util.List.of(";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << "Constants."
                << java_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "),\n";
        out << "            java.util.Optional.empty(),\n";
        out << "            java.util.List.of(),\n";
        out << "            java.util.List.of(),\n";
        out << "            java.util.List.of(),\n";
        out << "            java.util.List.of(),\n";
        out << "            java.util.Optional.empty(),\n";
        out << "            java.util.List.of()\n";
        out << "        );\n";
        out << "    }\n\n";
        out << "    public interface " << type_name << "Repository {\n";
        out << "        void registerDescriptor(com.statespec.backend.Backend backend) throws "
               "com.statespec.backend.Backend.BackendException;\n";
        out << "        java.util.Optional<com.statespec.backend.Backend.VersionedRecord> "
               "createTx(\n";
        out << "            com.statespec.backend.Backend.Transaction tx,\n";
        out << "            com.statespec.backend.Json document\n";
        out << "        ) throws com.statespec.backend.Backend.BackendException;\n";
        out << "        java.util.Optional<com.statespec.backend.Backend.VersionedRecord> getTx(\n";
        out << "            com.statespec.backend.Backend.Transaction tx,\n";
        out << "            java.util.List<com.statespec.generated.entities.EntityKeyValue> "
               "keyValues\n";
        out << "        ) throws com.statespec.backend.Backend.BackendException;\n";
        out << "        java.util.List<com.statespec.backend.Backend.VersionedRecord> "
               "listByIndexTx(\n";
        out << "            com.statespec.backend.Backend.Transaction tx,\n";
        out << "            String indexName,\n";
        out << "            java.util.List<com.statespec.backend.Backend.IndexValue> values\n";
        out << "        ) throws com.statespec.backend.Backend.BackendException;\n";
        for (const auto& index : entity.indexes)
        {
            out << "        java.util.List<com.statespec.backend.Backend.VersionedRecord> "
                << entity_index_repository_method_name(index.name) << "(\n";
            out << "            com.statespec.backend.Backend.Transaction tx,\n";
            out << "            java.util.List<com.statespec.backend.Backend.IndexValue> values\n";
            out << "        ) throws com.statespec.backend.Backend.BackendException;\n";
        }
        out << "        java.util.Optional<com.statespec.backend.Backend.VersionedRecord> "
               "updateTx(\n";
        out << "            com.statespec.backend.Backend.Transaction tx,\n";
        out << "            java.util.List<com.statespec.generated.entities.EntityKeyValue> "
               "keyValues,\n";
        out << "            com.statespec.backend.Json document,\n";
        out << "            long expectedVersion\n";
        out << "        ) throws com.statespec.backend.Backend.BackendException;\n";
        out << "    }\n\n";
        out << "    public static class Default" << type_name << "Repository implements "
            << type_name << "Repository {\n";
        out << "        private final com.statespec.generated.entities.EntityRepository "
               "entities = new com.statespec.generated.entities.DefaultEntityRepository();\n\n";
        out << "        @Override public void registerDescriptor(com.statespec.backend.Backend "
               "backend)\n";
        out << "            throws com.statespec.backend.Backend.BackendException\n";
        out << "        {\n";
        out << "            backend.ensureCollection(Schema." << lower_camel_identifier(entity.name)
            << "CollectionDescriptor());\n";
        out << "        }\n\n";
        out << "        @Override public java.util.Optional<com.statespec.backend.Backend."
               "VersionedRecord> createTx(\n";
        out << "            com.statespec.backend.Backend.Transaction tx,\n";
        out << "            com.statespec.backend.Json document\n";
        out << "        ) throws com.statespec.backend.Backend.BackendException\n";
        out << "        {\n";
        out << "            return entities.createEntityTx(\n";
        out << "                tx,\n";
        out << "                new com.statespec.generated.entities.EntityCreateRequest(\n";
        out << "                    "
               "com.statespec.generated.entities.EntityRepository.entityLookupFromDocument("
            << lower_camel_identifier(entity.name) << "EntityDescriptor(), document),\n";
        out << "                    document\n";
        out << "                )\n";
        out << "            );\n";
        out << "        }\n\n";
        out << "        @Override public java.util.Optional<com.statespec.backend.Backend."
               "VersionedRecord> getTx(\n";
        out << "            com.statespec.backend.Backend.Transaction tx,\n";
        out << "            java.util.List<com.statespec.generated.entities.EntityKeyValue> "
               "keyValues\n";
        out << "        ) throws com.statespec.backend.Backend.BackendException\n";
        out << "        {\n";
        out << "            return entities.getEntityTx(\n";
        out << "                tx, new com.statespec.generated.entities.EntityGetRequest("
               "build"
            << type_name << "Lookup(keyValues))\n";
        out << "            );\n";
        out << "        }\n\n";
        out << "        @Override public java.util.List<com.statespec.backend.Backend."
               "VersionedRecord> listByIndexTx(\n";
        out << "            com.statespec.backend.Backend.Transaction tx,\n";
        out << "            String indexName,\n";
        out << "            java.util.List<com.statespec.backend.Backend.IndexValue> values\n";
        out << "        ) throws com.statespec.backend.Backend.BackendException\n";
        out << "        {\n";
        out << "            return entities.listEntitiesByIndexTx(\n";
        out << "                tx,\n";
        out << "                new com.statespec.generated.entities.EntityListByIndexRequest("
               "Constants."
            << java_entity_name_constant_name(entity.name) << ", indexName, values)\n";
        out << "            );\n";
        out << "        }\n";
        for (const auto& index : entity.indexes)
        {
            out << "\n";
            out << "        @Override public java.util.List<com.statespec.backend.Backend."
                   "VersionedRecord> "
                << entity_index_repository_method_name(index.name) << "(\n";
            out << "            com.statespec.backend.Backend.Transaction tx,\n";
            out << "            java.util.List<com.statespec.backend.Backend.IndexValue> values\n";
            out << "        ) throws com.statespec.backend.Backend.BackendException\n";
            out << "        {\n";
            out << "            return listByIndexTx(tx, Constants."
                << java_entity_index_constant_name(entity.name, index.name) << ", values);\n";
            out << "        }\n";
        }
        out << "\n";
        out << "        @Override public java.util.Optional<com.statespec.backend.Backend."
               "VersionedRecord> updateTx(\n";
        out << "            com.statespec.backend.Backend.Transaction tx,\n";
        out << "            java.util.List<com.statespec.generated.entities.EntityKeyValue> "
               "keyValues,\n";
        out << "            com.statespec.backend.Json document,\n";
        out << "            long expectedVersion\n";
        out << "        ) throws com.statespec.backend.Backend.BackendException\n";
        out << "        {\n";
        out << "            return entities.upsertEntityTx(\n";
        out << "                tx,\n";
        out << "                new com.statespec.generated.entities.EntityUpsertRequest(\n";
        out << "                    build" << type_name << "Lookup(keyValues),\n";
        out << "                    document,\n";
        out << "                    java.util.Optional.of(expectedVersion)\n";
        out << "                )\n";
        out << "            );\n";
        out << "        }\n";
        out << "    }\n";
    }
    out << "}\n";
    return out.str();
}

std::string java_entity_constants_file(const IrEntity& entity)
{
    std::ostringstream out;
    out << "package com.statespec.generated.entities." << snake_identifier(entity.name) << ";\n\n";
    out << "/** Durable string constants for " << entity.name << ". */\n";
    out << "public final class Constants {\n";
    out << "    private Constants() {}\n\n";
    out << "    public static final String " << java_entity_name_constant_name(entity.name) << " = "
        << java_string(entity.name) << ";\n";
    out << "    public static final String " << java_entity_plural_name_constant_name(entity.name)
        << " = " << java_string(java_entity_plural_api_field_name(entity.name)) << ";\n";
    out << "    public static final String "
        << upper_snake_identifier(entity.name + "_collection_name") << " = "
        << java_string(entity.name) << ";\n";
    out << "    public static final String "
        << upper_snake_identifier(entity.name + "_key_helper_name") << " = "
        << java_string("build" + pascal_identifier(entity.name) + "Lookup") << ";\n";
    for (const auto& field : entity.fields)
    {
        out << "    public static final String "
            << java_entity_field_constant_name(entity.name, field.name) << " = "
            << java_string(field.name) << ";\n";
        out << "    public static final String "
            << java_entity_field_type_name_constant_name(entity.name, field.name) << " = "
            << java_string(field.type) << ";\n";
    }
    for (const auto& index : entity.indexes)
    {
        out << "    public static final String "
            << java_entity_index_constant_name(entity.name, index.name) << " = "
            << java_string(index.name) << ";\n";
        out << "    public static final String "
            << upper_snake_identifier(entity.name + "_index_" + index.name + "_helper_name")
            << " = " << java_string(entity_index_repository_method_name(index.name)) << ";\n";
    }
    for (const auto& state : entity.states)
    {
        out << "    public static final String "
            << java_entity_state_constant_name(entity.name, state.name) << " = "
            << java_string(state.name) << ";\n";
    }
    for (const auto& relation : entity.relations)
    {
        const auto relation_constant_prefix =
            upper_snake_identifier(entity.name + "_relation_" + relation.name);
        out << "    public static final String " << relation_constant_prefix
            << "_NAME = " << java_string(relation.name) << ";\n";
        out << "    public static final String " << relation_constant_prefix
            << "_TARGET = " << java_string(relation.target) << ";\n";
        out << "    public static final String " << relation_constant_prefix
            << "_KIND = " << java_string(relation.kind) << ";\n";
        if (relation.relation_kind.has_value())
        {
            out << "    public static final String " << relation_constant_prefix
                << "_RELATION_KIND = " << java_string(*relation.relation_kind) << ";\n";
        }
        if (relation.on_parent_delete.has_value())
        {
            out << "    public static final String " << relation_constant_prefix
                << "_ON_PARENT_DELETE = " << java_string(*relation.on_parent_delete) << ";\n";
        }
    }
    out << "}\n";
    return out.str();
}

TemplateRenderer::Values java_shape_descriptor_module_values(
    std::string_view package_name,
    std::string_view class_name,
    const IrSystem& system
)
{
    std::ostringstream content;
    content << "    public static List<ShapeDescriptor> shapeDescriptors() {\n";
    if (system.shapes.empty())
    {
        content << "        return List.of();\n";
        content << "    }\n\n";
        return java_descriptor_module_values(
            package_name, class_name, "shape descriptors", content.str()
        );
    }
    content << "        return List.of(\n";
    for (std::size_t i = 0; i < system.shapes.size(); ++i)
    {
        const auto& shape = system.shapes[i];
        content << "            com.statespec.generated.descriptors.shapes."
                << java_shape_descriptor_module_class_name(shape.name) << ".shapeDescriptors()";
        content << (i + 1 < system.shapes.size() ? ",\n" : "\n");
    }
    content << "        ).stream().flatMap(List::stream).toList();\n";
    content << "    }\n\n";
    return java_descriptor_module_values(
        package_name, class_name, "shape descriptors", content.str()
    );
}

TemplateRenderer::Values java_shape_descriptor_module_values(const IrShape& shape)
{
    IrSystem one_shape_system;
    one_shape_system.shapes.push_back(shape);
    return java_descriptor_module_values(
        "com.statespec.generated.descriptors.shapes",
        java_shape_descriptor_module_class_name(shape.name), "shape descriptor " + shape.name,
        generate_java_shape_descriptors(one_shape_system)
    );
}

void add_java_raw_common_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const std::filesystem::path& relative_output_path,
    std::string content
)
{
    const auto relative_path = common_artifact_path(relative_output_path.generic_string());
    result.files.push_back(
        GeneratedFile{
            (options.output_dir / relative_path).string(),
            std::move(content),
            GeneratedArtifactTier::Common,
            relative_path.generic_string(),
        }
    );
}

std::string java_shape_type_file(
    const IrShape& shape,
    std::string_view package_name
);

std::string java_shape_type_file(const IrShape& shape)
{
    return java_shape_type_file(shape, "com.statespec.generated.shapes");
}

std::string java_shape_type_file(
    const IrShape& shape,
    std::string_view package_name
)
{
    IrSystem one_shape_system;
    one_shape_system.shapes.push_back(shape);
    std::ostringstream out;
    out << "package " << package_name << ";\n\n";
    out << "import com.statespec.backend.Json;\n";
    if (package_name == "com.statespec.generated.shapes")
    {
        out << "import com.statespec.backend.Backend.FieldDescriptor;\n";
        out << "import com.statespec.backend.Backend.FieldType;\n";
        out << "import com.statespec.generated.descriptors.types.ShapeDescriptor;\n";
        out << "import java.util.List;\n";
    }
    out << "import java.util.Optional;\n\n";
    out << "public record " << pascal_identifier(shape.name) << "(\n";
    for (std::size_t i = 0; i < shape.fields.size(); ++i)
    {
        const auto& field = shape.fields[i];
        out << "    " << java_shape_type(field.type) << " " << field.name;
        out << (i + 1 < shape.fields.size() ? "," : "") << "\n";
    }
    out << ") {\n";
    if (package_name == "com.statespec.generated.shapes")
    {
        auto descriptors = generate_java_shape_descriptors(one_shape_system);
        out << "\n" << descriptors;
    }
    out << "}\n";
    return out.str();
}

const IrField* java_find_entity_field(
    const IrEntity& entity,
    const std::string& field_name
)
{
    const auto it = std::find_if(
        entity.fields.begin(), entity.fields.end(),
        [&](const IrField& field) { return field.name == field_name; }
    );
    return it == entity.fields.end() ? nullptr : &*it;
}

std::string java_entity_shape_field_descriptor_expr(
    const IrEntity& entity,
    const IrShape& shape,
    const IrField& field
)
{
    auto expression = java_shape_field_descriptor_expr(shape.name, field);
    if (java_find_entity_field(entity, field.name) == nullptr)
    {
        if (field.name == java_entity_plural_api_field_name(entity.name))
        {
            return "new FieldDescriptor(Constants." +
                   java_entity_plural_name_constant_name(entity.name) + ", " +
                   java_field_type_enum_expr(field.type) + ", " +
                   java_shape_field_type_name_constant_name(shape.name, field.name) + ", " +
                   (java_field_required(field.type) ? "true" : "false") + ")";
        }
        return expression;
    }
    expression = replace_all_copy(
        expression, java_shape_field_constant_name(shape.name, field.name),
        "Constants." + java_entity_field_constant_name(entity.name, field.name)
    );
    expression = replace_all_copy(
        expression, java_shape_field_type_name_constant_name(shape.name, field.name),
        "Constants." + java_entity_field_type_name_constant_name(entity.name, field.name)
    );
    return expression;
}

std::string java_entity_api_shape_descriptors(
    const IrEntity& entity,
    const std::vector<IrShape>& shapes
)
{
    std::ostringstream out;
    for (const auto& shape : shapes)
    {
        out << "    public static final String " << java_shape_name_constant_name(shape.name)
            << " = " << java_string(shape.name) << ";\n";
        for (const auto& field : shape.fields)
        {
            if (java_find_entity_field(entity, field.name) != nullptr)
            {
                continue;
            }
            if (field.name == java_entity_plural_api_field_name(entity.name))
            {
                out << "    public static final String "
                    << java_shape_field_type_name_constant_name(shape.name, field.name) << " = "
                    << java_string(field.type) << ";\n";
                continue;
            }
            out << "    public static final String "
                << java_shape_field_constant_name(shape.name, field.name) << " = "
                << java_string(field.name) << ";\n";
            out << "    public static final String "
                << java_shape_field_type_name_constant_name(shape.name, field.name) << " = "
                << java_string(field.type) << ";\n";
        }
    }
    if (!shapes.empty())
    {
        out << "\n";
    }

    out << "    public static List<ShapeDescriptor> shapeDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t shape_index = 0; shape_index < shapes.size(); ++shape_index)
    {
        const auto& shape = shapes[shape_index];
        out << "            new ShapeDescriptor(\n";
        out << "                " << java_shape_name_constant_name(shape.name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < shape.fields.size(); ++i)
        {
            const auto& field = shape.fields[i];
            out << "                    "
                << java_entity_shape_field_descriptor_expr(entity, shape, field);
            out << (i + 1 < shape.fields.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (shape_index + 1 < shapes.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";
    return out.str();
}

std::string java_entity_api_shapes_file(
    const IrEntity& entity,
    const std::vector<IrShape>& shapes,
    std::string_view package_name
)
{
    std::ostringstream out;
    out << "package " << package_name << ";\n\n";
    out << "import com.statespec.backend.Json;\n";
    out << "import com.statespec.backend.Backend.FieldDescriptor;\n";
    out << "import com.statespec.backend.Backend.FieldType;\n";
    out << "import com.statespec.backend.Backend.FieldType;\n";
    out << "import com.statespec.generated.descriptors.types.ShapeDescriptor;\n";
    out << "import java.util.List;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class Shapes {\n";
    out << "    private Shapes() {}\n\n";
    for (const auto& shape : shapes)
    {
        out << "    public record " << pascal_identifier(shape.name) << "(\n";
        for (std::size_t i = 0; i < shape.fields.size(); ++i)
        {
            const auto& field = shape.fields[i];
            out << "        " << java_shape_type(field.type) << " " << field.name;
            out << (i + 1 < shape.fields.size() ? "," : "") << "\n";
        }
        out << "    ) {}\n\n";
    }
    out << java_entity_api_shape_descriptors(entity, shapes);
    out << "}\n";
    return out.str();
}

std::string java_api_name_constant_name(const std::string& api_name)
{
    return upper_snake_identifier(api_name + "_api_name");
}

std::string java_api_route_name_constant_name(
    const std::string& api_server_name,
    const std::string& api_name
)
{
    return upper_snake_identifier(api_server_name + "_" + api_name + "_route_name");
}

std::string java_api_response_envelope_constant_name(const std::string& entity_name)
{
    return upper_snake_identifier(entity_name + "_list_response_envelope_name");
}

std::string java_api_server_name_constant_name(const std::string& api_server_name)
{
    return upper_snake_identifier(api_server_name + "_server_name");
}

std::string java_api_server_concurrency_constant_name(const std::string& api_server_name)
{
    return upper_snake_identifier(api_server_name + "_server_concurrency");
}

std::string java_api_server_constants_file(const IrApiServer& api_server)
{
    std::ostringstream out;
    out << "package com.statespec.generated.servers." << snake_identifier(api_server.name)
        << ";\n\n";
    out << "public final class Constants {\n";
    out << "    private Constants() {}\n\n";
    out << "    public static final String " << java_api_server_name_constant_name(api_server.name)
        << " = " << java_string(api_server.name) << ";\n";
    out << "    public static final int "
        << java_api_server_concurrency_constant_name(api_server.name) << " = "
        << api_server.concurrency.value_or(1) << ";\n";
    out << "}\n";
    return out.str();
}

std::string java_api_server_catalog_file(
    const IrSystem& system,
    const IrApiServer& api_server
)
{
    const auto entity_domains = crud_api_handler_domains_java(api_handler_domains(system));
    std::set<std::string> entity_api_names;
    std::ostringstream out;
    out << "package com.statespec.generated.servers." << snake_identifier(api_server.name)
        << ";\n\n";
    out << "import com.statespec.generated.descriptors.types.ApiServerDescriptor;\n";
    out << "import java.util.ArrayList;\n";
    out << "import java.util.List;\n\n";
    out << "public final class Catalog {\n";
    out << "    private Catalog() {}\n\n";
    for (const auto& domain : entity_domains)
    {
        for (const auto& api : domain.apis)
        {
            entity_api_names.insert(api.name);
        }
    }
    out << "    public static List<ApiServerDescriptor> apiServerDescriptors() {\n";
    out << "        var serves = new ArrayList<String>();\n";
    for (const auto& domain : entity_domains)
    {
        const auto served = std::any_of(
            domain.apis.begin(), domain.apis.end(),
            [&](const IrApi& api)
            {
                return std::find(api_server.serves.begin(), api_server.serves.end(), api.name) !=
                       api_server.serves.end();
            }
        );
        if (served)
        {
            out << "        com.statespec.generated.servers." << snake_identifier(api_server.name)
                << ".entities." << snake_identifier(domain.name)
                << ".Catalog.appendApiServerNames(serves);\n";
        }
    }
    for (const auto& served_api : api_server.serves)
    {
        if (entity_api_names.contains(served_api))
        {
            continue;
        }
        out << "        serves.add(" << java_string(served_api) << ");\n";
    }
    out << "        return List.of(new ApiServerDescriptor(\n";
    out << "            Constants." << java_api_server_name_constant_name(api_server.name) << ",\n";
    out << "            List.copyOf(serves),\n";
    out << "            Constants." << java_api_server_concurrency_constant_name(api_server.name)
        << "\n";
    out << "        ));\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string java_api_server_entity_catalog_file(
    const IrApiServer& api_server,
    const ApiHandlerDomain& domain
)
{
    std::vector<IrApi> served_domain_apis;
    for (const auto& api : domain.apis)
    {
        if (std::find(api_server.serves.begin(), api_server.serves.end(), api.name) !=
            api_server.serves.end())
        {
            served_domain_apis.push_back(api);
        }
    }

    std::ostringstream out;
    out << "package com.statespec.generated.servers." << snake_identifier(api_server.name)
        << ".entities." << snake_identifier(domain.name) << ";\n\n";
    out << "import java.util.List;\n\n";
    out << "public final class Catalog {\n";
    out << "    private Catalog() {}\n\n";
    out << "    public static void appendApiServerNames(List<String> serves) {\n";
    out << "        for (var apiName : com.statespec.generated.entities."
        << snake_identifier(domain.name) << ".Catalog.apiNames()) {\n";
    out << "            if (";
    for (std::size_t i = 0; i < served_domain_apis.size(); ++i)
    {
        if (i > 0)
        {
            out << " || ";
        }
        out << "apiName.equals(com.statespec.generated.entities." << snake_identifier(domain.name)
            << ".ApiConstants." << java_api_name_constant_name(served_domain_apis[i].name) << ")";
    }
    out << ") {\n";
    out << "                serves.add(apiName);\n";
    out << "            }\n";
    out << "        }\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string java_entity_api_constants_file(
    const IrSystem& system,
    const IrEntity& entity
)
{
    const auto shapes = java_entity_api_shapes(system, entity.name);
    std::vector<IrApi> apis;
    for (const auto& domain : crud_api_handler_domains_java(api_handler_domains(system)))
    {
        if (domain.name == entity.name)
        {
            apis = domain.apis;
            break;
        }
    }

    std::ostringstream out;
    out << "package com.statespec.generated.entities." << snake_identifier(entity.name) << ";\n\n";
    out << "public final class ApiConstants {\n";
    out << "    private ApiConstants() {}\n\n";
    for (const auto& api : apis)
    {
        out << "    public static final String " << java_api_name_constant_name(api.name) << " = "
            << java_string(api.name) << ";\n";
    }
    if (!apis.empty())
    {
        out << "\n";
    }
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api : apis)
        {
            if (std::find(api_server.serves.begin(), api_server.serves.end(), api.name) ==
                api_server.serves.end())
            {
                continue;
            }
            out << "    public static final String "
                << java_api_route_name_constant_name(api_server.name, api.name) << " = "
                << java_string(api_server.name + "." + api.name) << ";\n";
        }
    }
    if (!system.api_servers.empty() && !apis.empty())
    {
        out << "\n";
    }
    for (const auto& shape : shapes)
    {
        out << "    public static final String " << java_shape_name_constant_name(shape.name)
            << " = " << java_string(shape.name) << ";\n";
    }
    if (!shapes.empty())
    {
        out << "\n";
    }
    out << "    public static final String "
        << java_api_response_envelope_constant_name(entity.name) << " = Constants."
        << java_entity_plural_name_constant_name(entity.name) << ";\n";
    out << "}\n";
    return out.str();
}

std::string java_entity_api_catalog_file(
    const IrSystem& system,
    const IrEntity& entity
)
{
    std::vector<IrApi> apis;
    for (const auto& domain : crud_api_handler_domains_java(api_handler_domains(system)))
    {
        if (domain.name == entity.name)
        {
            apis = domain.apis;
            break;
        }
    }

    std::ostringstream out;
    out << "package com.statespec.generated.entities." << snake_identifier(entity.name) << ";\n\n";
    out << "import com.statespec.generated.ApiRequestContext;\n";
    out << "import com.statespec.generated.ApiResponse;\n";
    out << "import com.statespec.generated.descriptors.types.ApiDescriptor;\n";
    out << "import com.statespec.generated.descriptors.types.ApiRouteDescriptor;\n";
    out << "import com.statespec.generated.descriptors.types.ShapeDescriptor;\n";
    out << "import java.util.ArrayList;\n";
    out << "import java.util.List;\n\n";
    out << "public final class Catalog {\n";
    out << "    private Catalog() {}\n\n";
    out << "    public static List<ShapeDescriptor> shapeDescriptors() {\n";
    out << "        return Shapes.shapeDescriptors();\n";
    out << "    }\n\n";
    out << "    public static List<ApiDescriptor> apiDescriptors() {\n";
    out << "        var descriptors = new ArrayList<ApiDescriptor>();\n";
    for (const auto& api : apis)
    {
        out << "        descriptors.addAll(com.statespec.generated.entities."
            << snake_identifier(entity.name) << ".descriptors."
            << java_api_descriptor_module_class_name(api.name) << ".apiDescriptors());\n";
    }
    out << "        return List.copyOf(descriptors);\n";
    out << "    }\n\n";
    out << "    public static List<String> apiNames() {\n";
    out << "        return List.of(\n";
    for (std::size_t i = 0; i < apis.size(); ++i)
    {
        out << "            ApiConstants." << java_api_name_constant_name(apis[i].name);
        out << (i + 1 < apis.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";
    out << "    public static List<ApiRouteDescriptor> apiRouteDescriptors() {\n";
    out << "        var descriptors = new ArrayList<ApiRouteDescriptor>();\n";
    for (const auto& api : apis)
    {
        out << "        descriptors.addAll(com.statespec.generated.entities."
            << snake_identifier(entity.name) << ".descriptors."
            << java_api_descriptor_module_class_name(api.name) << ".apiRouteDescriptors());\n";
    }
    out << "        return List.copyOf(descriptors);\n";
    out << "    }\n\n";
    out << "    public static List<String> handlerEntrypoints() {\n";
    out << "        return List.of(\n";
    for (std::size_t i = 0; i < apis.size(); ++i)
    {
        out << "            \"handle" << pascal_identifier(apis[i].name) << "\"";
        out << (i + 1 < apis.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";
    for (const auto& api : apis)
    {
        out << "    public static ApiResponse handle" << pascal_identifier(api.name)
            << "(com.statespec.backend.Backend backend, ApiRequestContext context) "
               "throws Exception {\n";
        out << "        return Registry.handle" << pascal_identifier(api.name)
            << "(backend, context);\n";
        out << "    }\n\n";
    }
    out << "}\n";
    return out.str();
}

bool java_api_uses_shape(
    const IrSystem& system,
    std::string_view shape_name
)
{
    return std::any_of(
        system.apis.begin(), system.apis.end(),
        [&](const auto& api)
        {
            return (api.input.has_value() && *api.input == shape_name) ||
                   (api.output.has_value() && *api.output == shape_name);
        }
    );
}

const IrShape* java_find_shape(
    const IrSystem& system,
    std::string_view shape_name
)
{
    const auto it = std::find_if(
        system.shapes.begin(), system.shapes.end(),
        [&](const auto& shape) { return shape.name == shape_name; }
    );
    return it == system.shapes.end() ? nullptr : &*it;
}

IrSystem java_shapes_matching(
    const IrSystem& system,
    bool api_contract_shapes
)
{
    auto filtered = system;
    filtered.shapes.clear();
    for (const auto& shape : system.shapes)
    {
        if (java_api_uses_shape(system, shape.name) == api_contract_shapes)
        {
            filtered.shapes.push_back(shape);
        }
    }
    return filtered;
}

std::string java_api_shape_catalog_file(const IrSystem& system)
{
    std::ostringstream out;
    out << "package com.statespec.generated.shapes;\n\n";
    out << "import com.statespec.generated.descriptors.types.ShapeDescriptor;\n";
    out << "import java.util.ArrayList;\n";
    out << "import java.util.List;\n\n";
    out << "public final class ShapeCatalog {\n";
    out << "    private ShapeCatalog() {}\n\n";
    out << "    public static List<ShapeDescriptor> shapeDescriptors() {\n";
    out << "        var descriptors = new ArrayList<ShapeDescriptor>();\n";
    std::set<std::string> added_entities;
    for (const auto& shape : java_shapes_matching(system, true).shapes)
    {
        const auto owner = java_entity_api_shape_owner(system, shape.name);
        if (owner.has_value())
        {
            if (!added_entities.insert(*owner).second)
            {
                continue;
            }
            out << "        descriptors.addAll(com.statespec.generated.entities."
                << snake_identifier(*owner) << ".Catalog.shapeDescriptors());\n";
            continue;
        }
        out << "        descriptors.addAll(" << pascal_identifier(shape.name)
            << ".shapeDescriptors());\n";
    }
    out << "        return List.copyOf(descriptors);\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

void add_java_shape_type_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    const auto shape_path = join_artifact_path(GeneratedJavaOutputPackagePath, "shapes");
    for (const auto& shape : system.shapes)
    {
        if (java_api_uses_shape(system, shape.name))
        {
            continue;
        }
        add_java_raw_common_file(
            result, options, shape_path / (pascal_identifier(shape.name) + ".java"),
            java_shape_type_file(shape)
        );
    }
}

void add_java_api_shape_type_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    const auto shape_path = java_api_generated_path("shapes");
    add_java_raw_api_file(
        result, options, shape_path / "ShapeCatalog.java", java_api_shape_catalog_file(system)
    );
    for (const auto& shape : system.shapes)
    {
        if (!java_api_uses_shape(system, shape.name))
        {
            continue;
        }
        if (java_entity_api_shape_owner(system, shape.name).has_value())
        {
            continue;
        }
        add_java_raw_api_file(
            result, options, shape_path / (pascal_identifier(shape.name) + ".java"),
            java_shape_type_file(shape)
        );
    }
    for (const auto& entity : system.entities)
    {
        const auto shapes = java_entity_api_shapes(system, entity.name);
        if (shapes.empty())
        {
            continue;
        }
        const auto entity_package_path =
            java_api_generated_path("entities") / snake_identifier(entity.name);
        add_java_raw_api_file(
            result, options, entity_package_path / "Shapes.java",
            java_entity_api_shapes_file(
                entity, shapes, "com.statespec.generated.entities." + snake_identifier(entity.name)
            )
        );
    }
}

void add_java_workflow_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    const auto workflow_path = join_artifact_path(GeneratedJavaOutputPackagePath, "workflows");
    const std::string workflow_package = "com.statespec.generated.workflows";
    for (const auto& workflow : system.workflows)
    {
        const auto class_name = java_workflow_descriptor_module_class_name(workflow.name);
        add_java_raw_common_file(
            result, options, workflow_path / (class_name + ".java"),
            generate_java_workflow_descriptor(workflow, workflow_package, class_name)
        );
    }
}

std::string java_external_metadata_runtime_file()
{
    std::ostringstream out;
    out << "package com.statespec.generated.external.metadata;\n\n";
    out << "import com.statespec.backend.Backend;\n";
    out << "import com.statespec.backend.ExternalSystem;\n";
    out << "import com.statespec.backend.Json;\n";
    out << "import com.statespec.generated.descriptors.ExternalSystemDescriptorModule;\n";
    out << "import com.statespec.generated.descriptors.types.*;\n";
    out << "import java.util.List;\n";
    out << "import java.util.Map;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class ExternalSystemMetadata {\n";
    out << "    private ExternalSystemMetadata() {}\n\n";
    out << "    public static List<ExternalSystemMetadataMissingMappingSource> "
           "missingExternalSystemMetadataMappingSources(\n";
    out << "        ExternalSystemMetadataMappingPlan plan,\n";
    out << "        ExternalSystemMetadataMappingInputs inputs\n";
    out << "    ) {\n";
    out << "        java.util.ArrayList<ExternalSystemMetadataMissingMappingSource> "
           "missing = new java.util.ArrayList<>();\n";
    out << "        for (ExternalSystemMetadataMappingAssignment assignment : "
           "plan.allMappings()) {\n";
    out << "            if (inputs.assignmentValue(assignment).isEmpty()) {\n";
    out << "                missing.add(new ExternalSystemMetadataMissingMappingSource(\n";
    out << "                    assignment.source(),\n";
    out << "                    assignment.sourceRoot(),\n";
    out << "                    assignment.sourceField(),\n";
    out << "                    assignment.targetRoot(),\n";
    out << "                    assignment.field()\n";
    out << "                ));\n";
    out << "            }\n";
    out << "        }\n";
    out << "        return List.copyOf(missing);\n";
    out << "    }\n\n";
    out << "    public static Optional<String> "
           "externalSystemMetadataKey(ExternalSystem.MetadataLookup lookup) {\n";
    out << "        if (!lookup.keyComplete()) {\n";
    out << "            return Optional.empty();\n";
    out << "        }\n";
    out << "        java.util.HashMap<String, Json> values = new java.util.HashMap<>();\n";
    out << "        for (ExternalSystem.MetadataKeyValue keyValue : lookup.keyValues()) {\n";
    out << "            values.put(keyValue.field(), keyValue.value());\n";
    out << "        }\n";
    out << "        StringBuilder key = new StringBuilder();\n";
    out << "        for (String keyField : lookup.keyFields()) {\n";
    out << "            Json value = values.get(keyField);\n";
    out << "            if (value == null) {\n";
    out << "                return Optional.empty();\n";
    out << "            }\n";
    out << "            "
           "key.append(keyField).append('=').append(value.canonicalString()).append('\\n');\n";
    out << "        }\n";
    out << "        return Optional.of(key.toString());\n";
    out << "    }\n\n";
    out << "    public static Json externalSystemMetadataDocumentWithKeys(\n";
    out << "        Json document,\n";
    out << "        ExternalSystem.MetadataLookup lookup\n";
    out << "    ) {\n";
    out << "        java.util.HashMap<String, Json> values = new java.util.HashMap<>();\n";
    out << "        if (document instanceof Json.ObjectValue object) {\n";
    out << "            values.putAll(object.values());\n";
    out << "        }\n";
    out << "        for (ExternalSystem.MetadataKeyValue keyValue : lookup.keyValues()) {\n";
    out << "            values.put(keyValue.field(), keyValue.value());\n";
    out << "        }\n";
    out << "        return Json.object(values);\n";
    out << "    }\n\n";
    out << generate_java_external_system_metadata_runtime({});
    out << "}\n";
    return out.str();
}

void add_java_external_metadata_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options
)
{
    const auto metadata_path =
        join_artifact_path(GeneratedJavaOutputPackagePath, "external", "metadata");
    auto add_file = [&](std::string_view filename, std::string content)
    { add_java_raw_common_file(result, options, metadata_path / filename, std::move(content)); };
    auto package_header = []()
    { return std::string{"package com.statespec.generated.external.metadata;\n\n"}; };

    add_file(
        "ExternalSystemMetadataMappingAssignment.java",
        package_header() + "public record ExternalSystemMetadataMappingAssignment(\n"
                           "    String source,\n"
                           "    String sourceRoot,\n"
                           "    String sourceField,\n"
                           "    String targetRoot,\n"
                           "    String field\n"
                           ") {}\n"
    );
    add_file(
        "ExternalSystemMetadataMappingPlan.java",
        package_header() + "import java.util.List;\n\n"
                           "public record ExternalSystemMetadataMappingPlan(\n"
                           "    List<ExternalSystemMetadataMappingAssignment> allMappings,\n"
                           "    List<ExternalSystemMetadataMappingAssignment> clientMappings,\n"
                           "    List<ExternalSystemMetadataMappingAssignment> requestMappings\n"
                           ") {}\n"
    );
    add_file(
        "ExternalSystemMetadataMissingMappingSource.java",
        package_header() + "public record ExternalSystemMetadataMissingMappingSource(\n"
                           "    String source,\n"
                           "    String sourceRoot,\n"
                           "    String sourceField,\n"
                           "    String targetRoot,\n"
                           "    String field\n"
                           ") {}\n"
    );
    add_file(
        "ExternalSystemMetadataMappingInputs.java",
        package_header() +
            "import com.statespec.backend.Json;\n"
            "import java.util.Map;\n"
            "import java.util.Optional;\n\n"
            "public record ExternalSystemMetadataMappingInputs(\n"
            "    Map<String, Json> input,\n"
            "    Map<String, Json> entity,\n"
            "    Map<String, Json> workflow,\n"
            "    Map<String, Json> metadata\n"
            ") {\n"
            "    public Optional<Json> sourceValue(String sourceRoot, String sourceField) {\n"
            "        Map<String, Json> values = switch (sourceRoot) {\n"
            "            case \"input\" -> input;\n"
            "            case \"entity\" -> entity;\n"
            "            case \"workflow\" -> workflow;\n"
            "            case \"metadata\" -> metadata;\n"
            "            default -> null;\n"
            "        };\n"
            "        if (values == null) {\n"
            "            return Optional.empty();\n"
            "        }\n"
            "        return Optional.ofNullable(values.get(sourceField));\n"
            "    }\n\n"
            "    public Optional<Json> assignmentValue(\n"
            "        ExternalSystemMetadataMappingAssignment assignment\n"
            "    ) {\n"
            "        return sourceValue(assignment.sourceRoot(), assignment.sourceField());\n"
            "    }\n"
            "}\n"
    );
    add_file(
        "ExternalSystemMetadataMappingOutput.java",
        package_header() + "import com.statespec.backend.Json;\n"
                           "import java.util.List;\n"
                           "import java.util.Map;\n\n"
                           "public record ExternalSystemMetadataMappingOutput(\n"
                           "    Map<String, Json> clientConfig,\n"
                           "    Map<String, Json> requestPayload,\n"
                           "    List<ExternalSystemMetadataMissingMappingSource> missingSources\n"
                           ") {}\n"
    );
    add_file(
        "ExternalSystemMetadataMappingApplicator.java",
        package_header() +
            "public interface ExternalSystemMetadataMappingApplicator {\n"
            "    ExternalSystemMetadataMappingOutput applyExternalSystemMetadataMappings(\n"
            "        ExternalSystemMetadataMappingPlan plan,\n"
            "        ExternalSystemMetadataMappingInputs inputs\n"
            "    ) throws Exception;\n"
            "}\n"
    );
    add_file(
        "DefaultExternalSystemMetadataMappingApplicator.java",
        package_header() +
            "import com.statespec.backend.Json;\n"
            "import java.util.Map;\n\n"
            "public final class DefaultExternalSystemMetadataMappingApplicator\n"
            "    implements ExternalSystemMetadataMappingApplicator {\n"
            "    @Override public ExternalSystemMetadataMappingOutput "
            "applyExternalSystemMetadataMappings(\n"
            "        ExternalSystemMetadataMappingPlan plan,\n"
            "        ExternalSystemMetadataMappingInputs inputs\n"
            "    ) {\n"
            "        java.util.HashMap<String, Json> clientConfig = new java.util.HashMap<>();\n"
            "        java.util.HashMap<String, Json> requestPayload = new java.util.HashMap<>();\n"
            "        for (ExternalSystemMetadataMappingAssignment assignment : "
            "plan.clientMappings()) {\n"
            "            inputs.assignmentValue(assignment)\n"
            "                .ifPresent(value -> clientConfig.put(assignment.field(), value));\n"
            "        }\n"
            "        for (ExternalSystemMetadataMappingAssignment assignment : "
            "plan.requestMappings()) {\n"
            "            inputs.assignmentValue(assignment)\n"
            "                .ifPresent(value -> requestPayload.put(assignment.field(), value));\n"
            "        }\n"
            "        return new ExternalSystemMetadataMappingOutput(\n"
            "            Map.copyOf(clientConfig),\n"
            "            Map.copyOf(requestPayload),\n"
            "            ExternalSystemMetadata.missingExternalSystemMetadataMappingSources(\n"
            "                plan, inputs\n"
            "            )\n"
            "        );\n"
            "    }\n"
            "}\n"
    );
    add_file(
        "ExternalSystemOperatorMetadataUpsertRequest.java",
        package_header() + "import com.statespec.backend.ExternalSystem;\n"
                           "import com.statespec.backend.Json;\n"
                           "import java.util.Optional;\n\n"
                           "public record ExternalSystemOperatorMetadataUpsertRequest(\n"
                           "    ExternalSystem.MetadataLookup lookup,\n"
                           "    Json document,\n"
                           "    Optional<Long> expectedVersion\n"
                           ") {}\n"
    );
    add_file(
        "ExternalSystemOperatorMetadataGetRequest.java",
        package_header() + "import com.statespec.backend.ExternalSystem;\n\n"
                           "public record ExternalSystemOperatorMetadataGetRequest(\n"
                           "    ExternalSystem.MetadataLookup lookup\n"
                           ") {}\n"
    );
    add_file(
        "ExternalSystemOperatorMetadataDisableRequest.java",
        package_header() + "import com.statespec.backend.ExternalSystem;\n"
                           "import java.util.Optional;\n\n"
                           "public record ExternalSystemOperatorMetadataDisableRequest(\n"
                           "    ExternalSystem.MetadataLookup lookup,\n"
                           "    Optional<Long> expectedVersion,\n"
                           "    String disabledStatus\n"
                           ") {}\n"
    );
    add_file(
        "ExternalSystemOperatorMetadataDeleteRequest.java",
        package_header() + "import com.statespec.backend.ExternalSystem;\n"
                           "import java.util.Optional;\n\n"
                           "public record ExternalSystemOperatorMetadataDeleteRequest(\n"
                           "    ExternalSystem.MetadataLookup lookup,\n"
                           "    Optional<Long> expectedVersion,\n"
                           "    String deletedStatus\n"
                           ") {}\n"
    );
    add_file(
        "ExternalSystemOperatorMetadataRepository.java",
        package_header() + "import com.statespec.backend.Backend;\n"
                           "import java.util.Optional;\n\n"
                           "public interface ExternalSystemOperatorMetadataRepository {\n"
                           "    Optional<Backend.VersionedRecord> upsertMetadataTx(\n"
                           "        Backend.Transaction tx,\n"
                           "        ExternalSystemOperatorMetadataUpsertRequest request\n"
                           "    ) throws Backend.BackendException;\n\n"
                           "    Optional<Backend.VersionedRecord> getMetadataTx(\n"
                           "        Backend.Transaction tx,\n"
                           "        ExternalSystemOperatorMetadataGetRequest request\n"
                           "    ) throws Backend.BackendException;\n\n"
                           "    Optional<Backend.VersionedRecord> disableMetadataTx(\n"
                           "        Backend.Transaction tx,\n"
                           "        ExternalSystemOperatorMetadataDisableRequest request\n"
                           "    ) throws Backend.BackendException;\n\n"
                           "    Optional<Backend.VersionedRecord> deleteMetadataTx(\n"
                           "        Backend.Transaction tx,\n"
                           "        ExternalSystemOperatorMetadataDeleteRequest request\n"
                           "    ) throws Backend.BackendException;\n"
                           "}\n"
    );
    add_file(
        "DefaultExternalSystemOperatorMetadataRepository.java",
        package_header() +
            "import com.statespec.backend.Backend;\n"
            "import com.statespec.backend.ExternalSystem;\n"
            "import com.statespec.backend.Json;\n"
            "import java.util.Optional;\n\n"
            "public final class DefaultExternalSystemOperatorMetadataRepository implements\n"
            "    ExternalSystemOperatorMetadataRepository,\n"
            "    ExternalSystem {\n"
            "    @Override public Optional<Backend.VersionedRecord> upsertMetadataTx(\n"
            "        Backend.Transaction tx,\n"
            "        ExternalSystemOperatorMetadataUpsertRequest request\n"
            "    ) throws Backend.BackendException {\n"
            "        Optional<String> key = "
            "ExternalSystemMetadata.externalSystemMetadataKey(request.lookup());\n"
            "        if (key.isEmpty()) {\n"
            "            return Optional.empty();\n"
            "        }\n"
            "        Optional<Backend.VersionedRecord> existing =\n"
            "            tx.get(request.lookup().metadataEntity(), key.orElseThrow());\n"
            "        assertExpectedVersion(existing, request.expectedVersion());\n"
            "        tx.put(\n"
            "            request.lookup().metadataEntity(),\n"
            "            key.orElseThrow(),\n"
            "            ExternalSystemMetadata.externalSystemMetadataDocumentWithKeys(\n"
            "                request.document(), request.lookup()\n"
            "            )\n"
            "        );\n"
            "        return tx.get(request.lookup().metadataEntity(), key.orElseThrow());\n"
            "    }\n\n"
            "    @Override public Optional<Backend.VersionedRecord> getMetadataTx(\n"
            "        Backend.Transaction tx,\n"
            "        ExternalSystemOperatorMetadataGetRequest request\n"
            "    ) throws Backend.BackendException {\n"
            "        Optional<String> key = "
            "ExternalSystemMetadata.externalSystemMetadataKey(request.lookup());\n"
            "        return key.isPresent()\n"
            "            ? tx.get(request.lookup().metadataEntity(), key.orElseThrow())\n"
            "            : Optional.empty();\n"
            "    }\n\n"
            "    @Override public Optional<Backend.VersionedRecord> disableMetadataTx(\n"
            "        Backend.Transaction tx,\n"
            "        ExternalSystemOperatorMetadataDisableRequest request\n"
            "    ) throws Backend.BackendException {\n"
            "        return updateStatusTx(\n"
            "            tx, request.lookup(), request.expectedVersion(), "
            "request.disabledStatus()\n"
            "        );\n"
            "    }\n\n"
            "    @Override public Optional<Backend.VersionedRecord> deleteMetadataTx(\n"
            "        Backend.Transaction tx,\n"
            "        ExternalSystemOperatorMetadataDeleteRequest request\n"
            "    ) throws Backend.BackendException {\n"
            "        return updateStatusTx(\n"
            "            tx, request.lookup(), request.expectedVersion(), request.deletedStatus()\n"
            "        );\n"
            "    }\n\n"
            "    @Override public Optional<ExternalSystem.MetadataResolution> resolveMetadata(\n"
            "        Backend backend,\n"
            "        ExternalSystem.MetadataLookup lookup\n"
            "    ) throws Backend.BackendException {\n"
            "        Backend.Transaction tx = backend.begin();\n"
            "        try {\n"
            "            Optional<ExternalSystem.MetadataResolution> resolved = "
            "resolveMetadataTx(tx, lookup);\n"
            "            backend.commit(tx);\n"
            "            return resolved;\n"
            "        } catch (Backend.BackendException error) {\n"
            "            tx.abort();\n"
            "            throw error;\n"
            "        }\n"
            "    }\n\n"
            "    @Override public Optional<ExternalSystem.MetadataResolution> "
            "resolveMetadataTx(\n"
            "        Backend.Transaction tx,\n"
            "        ExternalSystem.MetadataLookup lookup\n"
            "    ) throws Backend.BackendException {\n"
            "        Optional<Backend.VersionedRecord> record =\n"
            "            getMetadataTx(tx, new ExternalSystemOperatorMetadataGetRequest(lookup));\n"
            "        return record.map(value -> new ExternalSystem.MetadataResolution(\n"
            "            value,\n"
            "            ExternalSystem.missingRequiredMetadataFields(\n"
            "                value.document(), lookup.requiredFields()\n"
            "            )\n"
            "        ));\n"
            "    }\n\n"
            "    private static void assertExpectedVersion(\n"
            "        Optional<Backend.VersionedRecord> existing,\n"
            "        Optional<Long> expectedVersion\n"
            "    ) throws Backend.BackendException {\n"
            "        if (expectedVersion.isEmpty()) {\n"
            "            return;\n"
            "        }\n"
            "        if (existing.isEmpty() || existing.orElseThrow().version() != "
            "expectedVersion.orElseThrow()) {\n"
            "            throw new Backend.ConflictException(\n"
            "                Backend.ConflictKind.VERSION_CONFLICT,\n"
            "                \"external system metadata version conflict\"\n"
            "            );\n"
            "        }\n"
            "    }\n\n"
            "    private static Optional<Backend.VersionedRecord> updateStatusTx(\n"
            "        Backend.Transaction tx,\n"
            "        ExternalSystem.MetadataLookup lookup,\n"
            "        Optional<Long> expectedVersion,\n"
            "        String status\n"
            "    ) throws Backend.BackendException {\n"
            "        Optional<String> key = "
            "ExternalSystemMetadata.externalSystemMetadataKey(lookup);\n"
            "        if (key.isEmpty()) {\n"
            "            return Optional.empty();\n"
            "        }\n"
            "        Optional<Backend.VersionedRecord> existing =\n"
            "            tx.get(lookup.metadataEntity(), key.orElseThrow());\n"
            "        if (existing.isEmpty()) {\n"
            "            return Optional.empty();\n"
            "        }\n"
            "        assertExpectedVersion(existing, expectedVersion);\n"
            "        java.util.HashMap<String, Json> values = new java.util.HashMap<>();\n"
            "        if (existing.orElseThrow().document() instanceof Json.ObjectValue object) {\n"
            "            values.putAll(object.values());\n"
            "        }\n"
            "        for (ExternalSystem.MetadataKeyValue keyValue : lookup.keyValues()) {\n"
            "            values.put(keyValue.field(), keyValue.value());\n"
            "        }\n"
            "        values.put(\"status\", Json.string(status));\n"
            "        tx.put(lookup.metadataEntity(), key.orElseThrow(), Json.object(values));\n"
            "        return tx.get(lookup.metadataEntity(), key.orElseThrow());\n"
            "    }\n"
            "}\n"
    );
    add_file(
        "ExternalSystemOperatorMetadataApiHandler.java",
        package_header() + "import com.statespec.backend.Backend;\n"
                           "import com.statespec.generated.ApiResponse;\n\n"
                           "public interface ExternalSystemOperatorMetadataApiHandler {\n"
                           "    ApiResponse handleUpsertMetadataTx(\n"
                           "        Backend.Transaction tx,\n"
                           "        ExternalSystemOperatorMetadataRepository repository,\n"
                           "        ExternalSystemOperatorMetadataUpsertRequest request\n"
                           "    ) throws Exception;\n\n"
                           "    ApiResponse handleGetMetadataTx(\n"
                           "        Backend.Transaction tx,\n"
                           "        ExternalSystemOperatorMetadataRepository repository,\n"
                           "        ExternalSystemOperatorMetadataGetRequest request\n"
                           "    ) throws Exception;\n\n"
                           "    ApiResponse handleDisableMetadataTx(\n"
                           "        Backend.Transaction tx,\n"
                           "        ExternalSystemOperatorMetadataRepository repository,\n"
                           "        ExternalSystemOperatorMetadataDisableRequest request\n"
                           "    ) throws Exception;\n\n"
                           "    ApiResponse handleDeleteMetadataTx(\n"
                           "        Backend.Transaction tx,\n"
                           "        ExternalSystemOperatorMetadataRepository repository,\n"
                           "        ExternalSystemOperatorMetadataDeleteRequest request\n"
                           "    ) throws Exception;\n"
                           "}\n"
    );
    add_file(
        "ExternalSystemCallRequest.java", package_header() +
                                              "import com.statespec.backend.Json;\n"
                                              "import java.util.Map;\n\n"
                                              "public record ExternalSystemCallRequest(\n"
                                              "    String externalSystem,\n"
                                              "    Map<String, Json> clientConfig,\n"
                                              "    Map<String, Json> requestPayload\n"
                                              ") {}\n"
    );
    add_file(
        "ExternalSystemCallResponse.java", package_header() +
                                               "import com.statespec.backend.Json;\n"
                                               "import java.util.Map;\n\n"
                                               "public record ExternalSystemCallResponse(\n"
                                               "    int statusCode,\n"
                                               "    Json body,\n"
                                               "    Map<String, Json> metadata\n"
                                               ") {}\n"
    );
    add_file(
        "ExternalSystemClient.java",
        package_header() +
            "public interface ExternalSystemClient {\n"
            "    ExternalSystemCallResponse callExternalSystem(ExternalSystemCallRequest request)\n"
            "        throws Exception;\n"
            "}\n"
    );
    add_file("ExternalSystemMetadata.java", java_external_metadata_runtime_file());
}

std::string java_api_shape_import(const IrSystem& system)
{
    std::ostringstream out;
    bool has_shared_api_shapes = false;
    for (const auto& api : system.apis)
    {
        const auto add_shape = [&](const std::optional<std::string>& shape_name)
        {
            if (!shape_name.has_value())
            {
                return;
            }
            if (!java_entity_api_shape_owner(system, *shape_name).has_value())
            {
                has_shared_api_shapes = true;
            }
        };
        add_shape(api.input);
        add_shape(api.output);
    }
    if (has_shared_api_shapes)
    {
        out << "import com.statespec.generated.shapes.*;\n";
    }
    for (const auto& entity : system.entities)
    {
        if (!java_entity_api_shapes(system, entity.name).empty())
        {
            out << "import static com.statespec.generated.entities."
                << snake_identifier(entity.name) << ".Shapes.*;\n";
        }
    }
    return out.str();
}

std::string java_api_default_handler_shape_import(const IrSystem& system)
{
    return java_api_shape_import(system);
}

void add_java_raw_api_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const std::filesystem::path& relative_output_path,
    std::string content
)
{
    const auto relative_path = artifact_path(relative_output_path.generic_string());
    result.files.push_back(
        GeneratedFile{
            (options.output_dir / relative_path).string(),
            std::move(content),
            GeneratedArtifactTier::Api,
            relative_path.generic_string(),
        }
    );
}

void add_java_raw_worker_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const std::filesystem::path& relative_output_path,
    std::string content
)
{
    const auto relative_path = artifact_path(relative_output_path.generic_string());
    result.files.push_back(
        GeneratedFile{
            (options.output_dir / relative_path).string(),
            std::move(content),
            GeneratedArtifactTier::Worker,
            relative_path.generic_string(),
        }
    );
}

bool java_api_server_serves(
    const IrApiServer& api_server,
    const std::string& api_name
)
{
    for (const auto& served_api : api_server.serves)
    {
        if (served_api == api_name)
        {
            return true;
        }
    }
    return false;
}

std::string java_api_optional_shape_name_expr(
    const IrSystem& system,
    const std::optional<std::string>& value
)
{
    if (!value.has_value())
    {
        return "Optional.empty()";
    }
    if (java_find_shape(system, *value) == nullptr)
    {
        return java_optional_string_expr(value);
    }
    const auto owner = java_entity_api_shape_owner(system, *value);
    if (owner.has_value())
    {
        return "Optional.of(Shapes." + java_shape_name_constant_name(*value) + ")";
    }
    return "Optional.of(" + pascal_identifier(*value) + "." +
           java_shape_name_constant_name(*value) + ")";
}

const IrEntity* java_api_entity(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api.entity.has_value())
    {
        return nullptr;
    }
    const auto it = std::find_if(
        system.entities.begin(), system.entities.end(),
        [&](const IrEntity& entity) { return entity.name == *api.entity; }
    );
    return it == system.entities.end() ? nullptr : &*it;
}

bool java_entity_has_field(
    const IrEntity& entity,
    std::string_view field_name
)
{
    return std::any_of(
        entity.fields.begin(), entity.fields.end(),
        [&](const IrField& field) { return field.name == field_name; }
    );
}

bool java_api_path_uses_entity_constants(
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = java_api_entity(system, api);
    if (entity == nullptr || !api.path.has_value())
    {
        return false;
    }
    for (std::size_t pos = 0; (pos = api.path->find('{', pos)) != std::string::npos;)
    {
        const auto end = api.path->find('}', pos + 1);
        if (end == std::string::npos)
        {
            return false;
        }
        if (java_entity_has_field(
                *entity, std::string_view{*api.path}.substr(pos + 1, end - pos - 1)
            ))
        {
            return true;
        }
        pos = end + 1;
    }
    return false;
}

std::string java_api_optional_path_expr(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api.path.has_value())
    {
        return "Optional.empty()";
    }
    const auto* entity = java_api_entity(system, api);
    if (entity == nullptr || !java_api_path_uses_entity_constants(system, api))
    {
        return java_optional_string_expr(api.path);
    }

    std::vector<std::string> parts;
    std::size_t cursor = 0;
    for (std::size_t pos = 0; (pos = api.path->find('{', cursor)) != std::string::npos;)
    {
        const auto end = api.path->find('}', pos + 1);
        if (end == std::string::npos)
        {
            return java_optional_string_expr(api.path);
        }
        const auto field_name = api.path->substr(pos + 1, end - pos - 1);
        parts.push_back(java_string(api.path->substr(cursor, pos - cursor + 1)));
        if (java_entity_has_field(*entity, field_name))
        {
            parts.push_back(
                "Constants." + java_entity_field_constant_name(entity->name, field_name)
            );
        }
        else
        {
            parts.push_back(java_string(field_name));
        }
        parts.push_back(java_string("}"));
        cursor = end + 1;
    }
    if (cursor < api.path->size())
    {
        parts.push_back(java_string(api.path->substr(cursor)));
    }
    std::ostringstream expr;
    expr << "Optional.of(";
    for (std::size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
        {
            expr << " + ";
        }
        expr << parts[i];
    }
    expr << ")";
    return expr.str();
}

std::string java_api_descriptor_module(
    const IrSystem& system,
    const IrApi& api
)
{
    std::ostringstream out;
    if (const auto* entity = java_api_entity(system, api); entity != nullptr)
    {
        out << "package com.statespec.generated.entities." << snake_identifier(entity->name)
            << ".descriptors;\n\n";
    }
    else
    {
        out << "package com.statespec.generated.descriptors;\n\n";
    }
    out << "import com.statespec.generated.descriptors.types.ApiDescriptor;\n";
    out << "import com.statespec.generated.descriptors.types.ApiRouteDescriptor;\n";
    if (const auto* entity = java_api_entity(system, api);
        entity != nullptr && java_api_path_uses_entity_constants(system, api))
    {
        out << "import com.statespec.generated.entities." << snake_identifier(entity->name)
            << ".Constants;\n";
    }
    std::set<std::string> imports;
    auto add_shape_import = [&](const std::optional<std::string>& shape_name)
    {
        if (!shape_name.has_value() || java_find_shape(system, *shape_name) == nullptr)
        {
            return;
        }
        const auto owner = java_entity_api_shape_owner(system, *shape_name);
        if (owner.has_value())
        {
            imports.insert(
                "import com.statespec.generated.entities." + snake_identifier(*owner) + ".Shapes;"
            );
        }
        else
        {
            imports.insert(
                "import com.statespec.generated.shapes." + pascal_identifier(*shape_name) + ";"
            );
        }
    };
    add_shape_import(api.input);
    add_shape_import(api.output);
    for (const auto& import : imports)
    {
        out << import << "\n";
    }
    out << "import java.util.List;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class " << java_api_descriptor_module_class_name(api.name) << " {\n";
    out << "    private " << java_api_descriptor_module_class_name(api.name) << "() {}\n\n";
    out << "    public static List<ApiDescriptor> apiDescriptors() {\n";
    out << "        return List.of(\n";
    out << "            new ApiDescriptor(\n";
    out << "                " << java_string(api.name) << ",\n";
    out << "                " << java_optional_string_expr(api.method) << ",\n";
    out << "                " << java_api_optional_path_expr(system, api) << ",\n";
    out << "                " << java_api_optional_shape_name_expr(system, api.input) << ",\n";
    out << "                " << java_api_optional_shape_name_expr(system, api.output) << ",\n";
    out << "                " << java_optional_string_expr(api.error) << ",\n";
    out << "                " << java_optional_string_expr(api.starts_workflow) << ",\n";
    out << "                " << java_optional_string_expr(api.enqueues) << "\n";
    out << "            )\n";
    out << "        );\n";
    out << "    }\n\n";
    out << "    public static List<ApiRouteDescriptor> apiRouteDescriptors() {\n";
    out << "        return List.of(\n";
    bool first_route = true;
    for (const auto& api_server : system.api_servers)
    {
        if (!java_api_server_serves(api_server, api.name))
        {
            continue;
        }
        if (!first_route)
        {
            out << ",\n";
        }
        first_route = false;
        out << "            new ApiRouteDescriptor(\n";
        out << "                " << java_string(api_server.name + "." + api.name) << ",\n";
        out << "                " << java_string(api_server.name) << ",\n";
        out << "                " << java_string(api.name) << ",\n";
        out << "                " << java_optional_string_expr(api.method) << ",\n";
        out << "                " << java_api_optional_path_expr(system, api) << ",\n";
        out << "                " << java_api_optional_shape_name_expr(system, api.input) << ",\n";
        out << "                " << java_api_optional_shape_name_expr(system, api.output) << ",\n";
        out << "                " << java_optional_string_expr(api.error) << "\n";
        out << "            )";
    }
    out << "\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string java_api_descriptor_catalog_file(const IrSystem& system);

void add_java_api_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    const auto descriptor_path = java_api_generated_path("descriptors");
    for (const auto& api : system.apis)
    {
        auto path = descriptor_path / (java_api_descriptor_module_class_name(api.name) + ".java");
        if (const auto* entity = java_api_entity(system, api); entity != nullptr)
        {
            path = java_api_generated_path("entities") / snake_identifier(entity->name) /
                   "descriptors" / (java_api_descriptor_module_class_name(api.name) + ".java");
        }
        add_java_raw_api_file(result, options, path, java_api_descriptor_module(system, api));
    }
    add_java_raw_api_file(
        result, options, descriptor_path / "Catalog.java", java_api_descriptor_catalog_file(system)
    );
}

std::string java_api_descriptor_catalog_file(const IrSystem& system)
{
    const auto entity_domains = crud_api_handler_domains_java(api_handler_domains(system));
    std::set<std::string> entity_api_names;
    std::ostringstream out;
    out << "package com.statespec.generated.descriptors;\n\n";
    out << "import com.statespec.generated.shapes.ShapeCatalog;\n";
    out << "import com.statespec.generated.descriptors.types.ApiDescriptor;\n";
    out << "import com.statespec.generated.descriptors.types.ApiRouteDescriptor;\n";
    out << "import com.statespec.generated.descriptors.types.ShapeDescriptor;\n";
    out << "import java.util.ArrayList;\n";
    out << "import java.util.List;\n\n";
    out << "public final class Catalog {\n";
    out << "    private Catalog() {}\n\n";
    out << "    public static List<ShapeDescriptor> shapeDescriptors() {\n";
    out << "        return ShapeCatalog.shapeDescriptors();\n";
    out << "    }\n\n";
    out << "    public static List<ApiDescriptor> apiDescriptors() {\n";
    out << "        var descriptors = new ArrayList<ApiDescriptor>();\n";
    for (const auto& domain : entity_domains)
    {
        out << "        descriptors.addAll(com.statespec.generated.entities."
            << snake_identifier(domain.name) << ".Catalog.apiDescriptors());\n";
        for (const auto& api : domain.apis)
        {
            entity_api_names.insert(api.name);
        }
    }
    for (const auto& api : system.apis)
    {
        if (entity_api_names.contains(api.name))
        {
            continue;
        }
        out << "        descriptors.addAll(" << java_api_descriptor_module_class_name(api.name)
            << ".apiDescriptors());\n";
    }
    out << "        return List.copyOf(descriptors);\n";
    out << "    }\n\n";
    out << "    public static List<ApiRouteDescriptor> apiRouteDescriptors() {\n";
    out << "        var descriptors = new ArrayList<ApiRouteDescriptor>();\n";
    for (const auto& domain : entity_domains)
    {
        out << "        descriptors.addAll(com.statespec.generated.entities."
            << snake_identifier(domain.name) << ".Catalog.apiRouteDescriptors());\n";
    }
    for (const auto& api : system.apis)
    {
        if (entity_api_names.contains(api.name))
        {
            continue;
        }
        out << "        descriptors.addAll(" << java_api_descriptor_module_class_name(api.name)
            << ".apiRouteDescriptors());\n";
    }
    out << "        return List.copyOf(descriptors);\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string java_api_descriptors_file(const IrSystem& system)
{
    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << "import com.statespec.generated.descriptors.Catalog;\n";
    out << "import com.statespec.generated.descriptors.types.ApiDescriptor;\n";
    out << "import com.statespec.generated.descriptors.types.ApiRouteDescriptor;\n";
    out << "import com.statespec.generated.descriptors.types.ApiServerDescriptor;\n";
    out << "import java.util.ArrayList;\n";
    out << "import java.util.List;\n\n";
    out << "public final class ApiDescriptors {\n";
    out << "    private ApiDescriptors() {}\n\n";
    out << "    public static List<ApiDescriptor> apiDescriptors() {\n";
    out << "        return Catalog.apiDescriptors();\n";
    out << "    }\n\n";
    out << "    public static List<ApiServerDescriptor> apiServerDescriptors() {\n";
    out << "        var descriptors = new ArrayList<ApiServerDescriptor>();\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "        descriptors.addAll(com.statespec.generated.servers."
            << snake_identifier(api_server.name) << ".Catalog.apiServerDescriptors());\n";
    }
    out << "        return List.copyOf(descriptors);\n";
    out << "    }\n\n";
    out << "    public static List<ApiRouteDescriptor> apiRouteDescriptors() {\n";
    out << "        return Catalog.apiRouteDescriptors();\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string java_worker_descriptor_catalog_file(const IrSystem& system)
{
    std::ostringstream out;
    out << "package com.statespec.generated.worker.descriptors;\n\n";
    out << "import com.statespec.generated.descriptors.types.WorkerContext;\n";
    out << "import com.statespec.generated.descriptors.types.WorkerDescriptor;\n";
    out << "import java.util.ArrayList;\n";
    out << "import java.util.List;\n\n";
    out << "public final class Catalog {\n";
    out << "    private Catalog() {}\n\n";
    out << "    public static List<WorkerDescriptor> workerDescriptors() {\n";
    out << "        var descriptors = new ArrayList<WorkerDescriptor>();\n";
    for (const auto& worker : system.workers)
    {
        out << "        descriptors.add(" << java_worker_descriptor_module_class_name(worker.name)
            << ".workerDescriptor());\n";
    }
    out << "        return List.copyOf(descriptors);\n";
    out << "    }\n\n";
    out << "    public static List<WorkerContext> workerContexts() {\n";
    out << "        var contexts = new ArrayList<WorkerContext>();\n";
    for (const auto& worker : system.workers)
    {
        out << "        contexts.add(" << java_worker_descriptor_module_class_name(worker.name)
            << ".workerContext());\n";
    }
    out << "        return List.copyOf(contexts);\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

void add_java_descriptor_module_artifact(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const std::filesystem::path& relative_output_path,
    std::string_view package_name,
    std::string_view class_name,
    std::string_view module_name,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
)
{
    auto resolved_values =
        values.empty() ? java_descriptor_module_values(package_name, class_name, module_name)
                       : values;
    add_generated_template_file(
        result, options.output_dir, templates,
        generated_template_path("DescriptorModule.java.tmpl"),
        common_artifact_path(relative_output_path.generic_string()), diagnostics,
        GeneratedArtifactTier::Common, resolved_values
    );
}

void add_java_descriptor_module_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto descriptor_package_path =
        join_artifact_path(GeneratedJavaOutputPackagePath, "descriptors");
    const auto entity_descriptor_package_path = descriptor_package_path / "entities";
    const auto shape_descriptor_package_path = descriptor_package_path / "shapes";
    const auto descriptor_type_package_path = descriptor_package_path / "types";
    const std::string descriptor_package = "com.statespec.generated.descriptors";
    const std::string entity_descriptor_package = descriptor_package + ".entities";
    const auto generated_package_path = artifact_path(GeneratedJavaOutputPackagePath);

    add_java_raw_common_file(
        result, options, descriptor_package_path / "DescriptorCatalog.java",
        "package com.statespec.generated.descriptors;\n\n"
        "public final class DescriptorCatalog {\n"
        "    private DescriptorCatalog() {}\n"
        "}\n"
    );
    for (const auto& [filename, content] : java_descriptor_type_files())
    {
        add_java_raw_common_file(result, options, descriptor_type_package_path / filename, content);
    }
    const auto entity_runtime_package_path = generated_package_path / "entities";
    const std::array<std::string_view, 9> entity_runtime_files{
        "EntityKeyValue.java",           "EntityLookup.java",        "EntityCreateRequest.java",
        "EntityGetRequest.java",         "EntityUpsertRequest.java", "EntityDeleteRequest.java",
        "EntityListByIndexRequest.java", "EntityRepository.java",    "DefaultEntityRepository.java",
    };
    for (const auto filename : entity_runtime_files)
    {
        const std::string template_filename = std::string{filename} + ".tmpl";
        add_generated_template_file(
            result, options.output_dir, templates, generated_template_path(template_filename),
            common_artifact_path((entity_runtime_package_path / filename).generic_string()),
            diagnostics, GeneratedArtifactTier::Common
        );
    }
    add_java_raw_common_file(
        result, options, generated_package_path / "runtime" / "RuntimeRegistration.java",
        "package com.statespec.generated.runtime;\n\n"
        "import com.statespec.backend.Backend;\n"
        "import com.statespec.backend.FeatureFlag;\n"
        "import com.statespec.backend.Lease;\n"
        "import com.statespec.backend.Log;\n"
        "import com.statespec.backend.Metric;\n"
        "import com.statespec.backend.Queue;\n"
        "import com.statespec.backend.Workflow;\n"
        "import com.statespec.generated.descriptors.types.*;\n"
        "import java.util.List;\n\n"
        "import static com.statespec.generated.Descriptors.*;\n\n"
        "public final class RuntimeRegistration {\n"
        "    private RuntimeRegistration() {}\n\n" +
            generate_java_runtime_registration(system, templates) + "}\n"
    );
    add_java_raw_common_file(
        result, options, generated_package_path / "ApiRequestContext.java",
        "package com.statespec.generated;\n\n"
        "import com.statespec.backend.Json;\n"
        "import java.util.Optional;\n\n"
        "public record ApiRequestContext(\n"
        "    String serverName,\n"
        "    String apiName,\n"
        "    Optional<String> method,\n"
        "    Optional<String> path,\n"
        "    Json body\n"
        ") {}\n"
    );
    add_java_raw_common_file(
        result, options, generated_package_path / "ApiResponse.java",
        "package com.statespec.generated;\n\n"
        "import com.statespec.backend.Json;\n\n"
        "public record ApiResponse(\n"
        "    int statusCode,\n"
        "    Json body\n"
        ") {}\n"
    );
    add_java_external_metadata_artifacts(result, options);

    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "CoreDescriptorModule.java",
        descriptor_package, "CoreDescriptorModule", "core descriptors", diagnostics,
        java_descriptor_module_values(
            descriptor_package, "CoreDescriptorModule", "core descriptors",
            generate_java_value_enum_descriptors(system)
        )
    );
    add_java_raw_common_file(
        result, options, descriptor_package_path / "EventDescriptorModule.java",
        java_event_descriptor_module_file(system)
    );
    add_java_raw_common_file(
        result, options, descriptor_package_path / "ExternalSystemDescriptorModule.java",
        java_external_system_descriptor_module_file(system, templates)
    );
    const auto common_shapes = java_shapes_matching(system, false);
    if (!common_shapes.shapes.empty())
    {
        add_java_descriptor_module_artifact(
            result, options, templates, descriptor_package_path / "ShapeDescriptorModule.java",
            descriptor_package, "ShapeDescriptorModule", "shape descriptors", diagnostics,
            java_shape_descriptor_module_values(
                descriptor_package, "ShapeDescriptorModule", common_shapes
            )
        );
        for (const auto& shape : common_shapes.shapes)
        {
            const auto class_name = java_shape_descriptor_module_class_name(shape.name);
            add_generated_template_file(
                result, options.output_dir, templates,
                generated_template_path("DescriptorModule.java.tmpl"),
                common_artifact_path(
                    (shape_descriptor_package_path / (class_name + ".java")).generic_string()
                ),
                diagnostics, GeneratedArtifactTier::Common,
                java_shape_descriptor_module_values(shape)
            );
        }
    }
    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "PolicyDescriptorModule.java",
        descriptor_package, "PolicyDescriptorModule", "policy descriptors", diagnostics,
        java_descriptor_module_values(
            descriptor_package, "PolicyDescriptorModule", "policy descriptors",
            generate_java_policy_descriptors(system)
        )
    );
    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "RuntimeDescriptorModule.java",
        descriptor_package, "RuntimeDescriptorModule", "runtime descriptors", diagnostics,
        java_descriptor_module_values(
            descriptor_package, "RuntimeDescriptorModule", "runtime descriptors",
            generate_java_feature_flag_descriptors(system) +
                generate_java_runtime_descriptors(system)
        )
    );
    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "ObservabilityDescriptorModule.java",
        descriptor_package, "ObservabilityDescriptorModule", "observability descriptors",
        diagnostics,
        java_descriptor_module_values(
            descriptor_package, "ObservabilityDescriptorModule", "observability descriptors",
            generate_java_observability_descriptors(system)
        )
    );
    for (const auto& [name, class_name] : java_runtime_registration_modules(system))
    {
        add_java_raw_common_file(
            result, options, descriptor_package_path / (class_name + ".java"),
            java_runtime_registration_module_file(templates, name, class_name)
        );
    }
    for (const auto& entity : system.entities)
    {
        const auto entity_uses_gc = std::any_of(
            entity.states.begin(), entity.states.end(),
            [](const IrState& state) { return state.garbage_collection.has_value(); }
        );
        const auto entity_path = std::filesystem::path{"com"} / "statespec" / "generated" /
                                 "entities" / snake_identifier(entity.name);
        add_java_raw_common_file(
            result, options, entity_path / "Constants.java", java_entity_constants_file(entity)
        );
        add_java_raw_common_file(
            result, options, entity_path / "Model.java",
            java_entity_centered_facade_file(entity, "Model", "model")
        );
        add_java_raw_common_file(
            result, options, entity_path / "Persistence.java",
            java_entity_centered_facade_file(entity, "Persistence", "persistence")
        );
        add_java_raw_common_file(
            result, options, entity_path / "Schema.java",
            java_entity_centered_facade_file(entity, "Schema", "schema")
        );
        if (entity_uses_gc)
        {
            add_java_raw_common_file(
                result, options, entity_path / "Gc.java", java_entity_gc_descriptor_file(entity)
            );
        }
    }
}

} // namespace

void add_java_common_runtime_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto usage = runtime_domain_usage(system);
    const auto output_root = artifact_path(GeneratedJavaBackendPackagePath);

    add_template_file(
        result, options.output_dir, templates, output_root / "Json.java", output_root / "Json.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Backend.java",
        output_root / "Backend.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "ExternalSystem.java",
        output_root / "ExternalSystem.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "FeatureFlag.java",
        output_root / "FeatureFlag.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Lease.java",
        output_root / "Lease.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Log.java", output_root / "Log.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Metric.java",
        output_root / "Metric.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Queue.java",
        output_root / "Queue.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "SchemaCompatibility.java",
        output_root / "SchemaCompatibility.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Workflow.java",
        output_root / "Workflow.java", diagnostics
    );

    if (diagnostics.has_errors())
    {
        return;
    }

    add_template_file(
        result, options.output_dir, templates, output_root / "memory" / "InMemoryBackend.java",
        output_root / "memory" / "InMemoryBackend.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "memory" / "InMemoryTransaction.java",
        output_root / "memory" / "InMemoryTransaction.java", diagnostics
    );
    if (usage.uses_any_runtime_domain)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "Codec.java",
            output_root / "runtime" / "Codec.java", diagnostics
        );
    }
    if (usage.uses_feature_flags)
    {
        add_template_file(
            result, options.output_dir, templates,
            output_root / "runtime" / "FeatureFlagCodec.java",
            output_root / "runtime" / "FeatureFlagCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates,
            output_root / "runtime" / "FeatureFlagStore.java",
            output_root / "runtime" / "FeatureFlagStore.java", diagnostics
        );
    }
    if (usage.uses_queues)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "QueueCodec.java",
            output_root / "runtime" / "QueueCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "QueueStore.java",
            output_root / "runtime" / "QueueStore.java", diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "LeaseCodec.java",
            output_root / "runtime" / "LeaseCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "LeaseStore.java",
            output_root / "runtime" / "LeaseStore.java", diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "WorkflowCodec.java",
            output_root / "runtime" / "WorkflowCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "WorkflowStore.java",
            output_root / "runtime" / "WorkflowStore.java", diagnostics
        );
    }
    if (usage.uses_observability)
    {
        add_template_file(
            result, options.output_dir, templates,
            output_root / "runtime" / "ObservabilityCodec.java",
            output_root / "runtime" / "ObservabilityCodec.java", diagnostics
        );
    }
    if (usage.uses_logs)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "LogCodec.java",
            output_root / "runtime" / "LogCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "LogSink.java",
            output_root / "runtime" / "LogSink.java", diagnostics
        );
    }
    if (usage.uses_metrics)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "MetricCodec.java",
            output_root / "runtime" / "MetricCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "MetricSink.java",
            output_root / "runtime" / "MetricSink.java", diagnostics
        );
    }
    if (usage.uses_entity_gc)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            template_path_for_output(output_root / "runtime" / "EntityGcTypes.java"),
            common_artifact_path((output_root / "runtime" / "EntityGcTypes.java").generic_string()),
            diagnostics, GeneratedArtifactTier::Common
        );
        add_template_file(
            result, options.output_dir, templates,
            output_root / "runtime" / "EntityGcRepository.java",
            output_root / "runtime" / "EntityGcRepository.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "EntityGcWorkers.java",
            output_root / "runtime" / "EntityGcWorkers.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates,
            output_root / "runtime" / "EntityGcRegistration.java",
            output_root / "runtime" / "EntityGcRegistration.java", diagnostics
        );
    }
    add_java_descriptor_module_artifacts(result, options, templates, system, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }
    add_java_shape_type_artifacts(result, options, system);
    add_java_workflow_descriptor_artifacts(result, options, system);
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Descriptors.java.tmpl"),
        common_artifact_path(
            join_artifact_path(GeneratedJavaOutputPackagePath, "Descriptors.java").generic_string()
        ),
        diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_java(system, templates)}}
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Makefile.tmpl"),
        artifact_path(GeneratedArtifactMakefile), diagnostics, GeneratedArtifactTier::Common,
        java_makefile_values(options.tier, system), common_artifact_path(GeneratedArtifactMakefile)
    );
}

void add_java_api_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    if (system.apis.empty() && system.api_servers.empty())
    {
        return;
    }

    const auto include_api_composition = !system.api_servers.empty();

    add_java_api_shape_type_artifacts(result, options, system);
    add_java_api_descriptor_artifacts(result, options, system);
    for (const auto& api_server : system.api_servers)
    {
        add_java_raw_api_file(
            result, options,
            "api/com/statespec/generated/servers/" + snake_identifier(api_server.name) +
                "/Constants.java",
            java_api_server_constants_file(api_server)
        );
        for (const auto& domain : crud_api_handler_domains_java(api_handler_domains(system)))
        {
            const auto served = std::any_of(
                domain.apis.begin(), domain.apis.end(),
                [&](const IrApi& api)
                {
                    return std::find(
                               api_server.serves.begin(), api_server.serves.end(), api.name
                           ) != api_server.serves.end();
                }
            );
            if (!served)
            {
                continue;
            }
            add_java_raw_api_file(
                result, options,
                "api/com/statespec/generated/servers/" + snake_identifier(api_server.name) +
                    "/entities/" + snake_identifier(domain.name) + "/Catalog.java",
                java_api_server_entity_catalog_file(api_server, domain)
            );
        }
        add_java_raw_api_file(
            result, options,
            "api/com/statespec/generated/servers/" + snake_identifier(api_server.name) +
                "/Catalog.java",
            java_api_server_catalog_file(system, api_server)
        );
    }
    add_java_raw_api_file(
        result, options, java_api_generated_path("ApiDescriptors.java"),
        java_api_descriptors_file(system)
    );
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiCodecs.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_codec_imports", java_api_codec_imports(system)},
            {"api_shape_import", java_api_shape_import(system)},
            {"api_codec_helpers", generate_api_codec_helpers_java()},
            {"api_codec_delegates", java_api_codec_delegates(system)}
        }
    );
    for (const auto& shape : api_codec_shapes(system))
    {
        if (java_entity_api_shape_owner(system, shape.name).has_value())
        {
            continue;
        }
        add_java_raw_api_file(
            result, options, java_api_codec_shape_path(shape.name),
            java_api_codec_shape_file(system, shape)
        );
    }
    for (const auto& entity : system.entities)
    {
        if (java_entity_api_shapes(system, entity.name).empty())
        {
            continue;
        }
        add_java_raw_api_file(
            result, options,
            "api/com/statespec/generated/entities/" + snake_identifier(entity.name) +
                "/ApiConstants.java",
            java_entity_api_constants_file(system, entity)
        );
        add_java_raw_api_file(
            result, options, java_entity_api_codec_path(entity.name),
            java_entity_api_codec_file(system, entity)
        );
    }
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiHandlers.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"business_api_operation_handler_methods",
             generate_business_api_operation_handler_methods_java(system)}
        }
    );
    const auto handler_domains = crud_api_handler_domains_java(api_handler_domains(system));
    for (const auto& domain : handler_domains)
    {
        for (const auto& entity : system.entities)
        {
            if (entity.name != domain.name)
            {
                continue;
            }
            add_java_raw_api_file(
                result, options, java_entity_api_catalog_path(domain.name),
                java_entity_api_catalog_file(system, entity)
            );
            break;
        }
        add_java_raw_api_file(
            result, options, java_api_handler_domain_path(domain.name),
            java_api_handler_domain_file(system, domain)
        );
        add_java_raw_api_file(
            result, options, java_api_handler_domain_registry_path(domain.name),
            java_api_handler_domain_registry_file(domain)
        );
    }
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiHandlerRegistry.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_handler_registry_domain_imports",
             java_api_handler_registry_domain_imports(handler_domains)},
            {"api_handler_lookup_registrations",
             java_api_handler_lookup_registrations(handler_domains)},
            {"api_business_handler_lookup_entries",
             java_business_api_handler_lookup_entries(system)}
        }
    );
    add_java_generated_template_file(
        result, options, templates,
        java_api_generated_path("ExternalSystemOperatorMetadataApi.java"),
        GeneratedArtifactTier::Api, diagnostics
    );
    if (include_api_composition)
    {
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiApplication.java"),
            GeneratedArtifactTier::Api, diagnostics, java_api_runtime_bootstrap_values(system)
        );
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiProcess.java"),
            GeneratedArtifactTier::Api, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiDispatcher.java"),
            GeneratedArtifactTier::Api, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiServer.java"),
            GeneratedArtifactTier::Api, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiTransport.java"),
            GeneratedArtifactTier::Api, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiRoutes.java"),
            GeneratedArtifactTier::Api, diagnostics
        );
        if (runtime_domain_usage(system).uses_entity_gc)
        {
            add_java_raw_api_file(
                result, options, java_api_generated_path("EntityGcCatalog.java"),
                java_api_entity_gc_catalog_file(system)
            );
        }
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiMain.java"),
            GeneratedArtifactTier::Api, diagnostics, java_api_main_values(system)
        );
    }
}

void add_java_worker_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto usage = runtime_domain_usage(system);
    const auto include_worker_composition = !system.workers.empty();
    const auto include_worker_execution = include_worker_composition || usage.uses_workflows;
    if (!include_worker_execution)
    {
        return;
    }

    add_java_generated_template_file(
        result, options, templates, java_worker_generated_path("WorkerDescriptors.java"),
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_java_generated_template_file(
        result, options, templates, java_worker_generated_path("WorkerContexts.java"),
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_java_raw_worker_file(
        result, options, java_worker_generated_path("worker/descriptors/Catalog.java"),
        java_worker_descriptor_catalog_file(system)
    );
    for (const auto& worker : system.workers)
    {
        const auto class_name = java_worker_descriptor_module_class_name(worker.name);
        add_java_raw_worker_file(
            result, options,
            java_worker_generated_path("worker/descriptors/" + class_name + ".java"),
            generate_java_worker_descriptor_module(
                worker, "com.statespec.generated.worker.descriptors", class_name
            )
        );
    }
    add_java_raw_worker_file(
        result, options, java_worker_generated_path("WorkerRegistry.java"),
        java_worker_registry_facade_file(system)
    );
    for (const auto& worker : system.workers)
    {
        add_java_raw_worker_file(
            result, options,
            java_worker_generated_path(
                "registry/" + pascal_identifier(worker.name) + "Registry.java"
            ),
            java_worker_registry_module_file(
                worker, "com.statespec.generated.registry",
                pascal_identifier(worker.name) + "Registry"
            )
        );
    }
    if (include_worker_composition)
    {
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerApplication.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerProcess.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerRuntime.java"),
            GeneratedArtifactTier::Worker, diagnostics, java_runtime_bootstrap_values(system)
        );
        if (usage.uses_entity_gc)
        {
            add_java_raw_worker_file(
                result, options, java_worker_generated_path("WorkerEntityGcCatalog.java"),
                java_worker_entity_gc_catalog_file(system)
            );
        }
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerMain.java"),
            GeneratedArtifactTier::Worker, diagnostics, java_worker_main_values(system)
        );
    }
    if (include_worker_execution)
    {
        for (const auto& workflow : system.workflows)
        {
            add_java_raw_worker_file(
                result, options,
                java_worker_generated_path(
                    "workflows/" + snake_identifier(workflow.name) + "/Handlers.java"
                ),
                generate_java_workflow_handler_module(workflow)
            );
            add_java_raw_worker_file(
                result, options,
                java_worker_generated_path(
                    "workflows/" + java_workflow_worker_module_class_name(workflow) + ".java"
                ),
                generate_java_workflow_worker_module(workflow)
            );
        }
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkflowStepHandlers.java"),
            GeneratedArtifactTier::Worker, diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_handler_imports", java_workflow_step_handler_imports(system)},
                {"workflow_step_handler_bases", java_workflow_step_handler_bases(system)},
                {"workflow_step_handler_methods", std::string{}},
                {"default_workflow_step_handler_methods",
                 generate_default_workflow_step_handler_methods_java(system)},
                {"workflow_step_handler_keys", generate_workflow_step_handler_keys_java(system)}
            }
        );
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkflowRunner.java"),
            GeneratedArtifactTier::Worker, diagnostics, java_workflow_runner_values(system)
        );
    }
    if (usage.uses_queues)
    {
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerQueues.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerLeases.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerWorkflows.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
}

} // namespace statespec
