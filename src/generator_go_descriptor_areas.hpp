#pragma once

#include "statespec/generator_bindings.hpp"
#include "statespec/template_renderer.hpp"

#include <string>
#include <string_view>

namespace statespec
{

std::string generate_go_descriptor_prelude(
    const IrSystem& system,
    const std::string& external_system_runtime,
    const std::string& external_system_metadata_runtime,
    const std::string& entity_repository_runtime
);
std::string generate_go_feature_flag_descriptors(const IrSystem& system);
std::string generate_go_declaration_descriptors(const IrSystem& system);
std::string generate_go_external_system_descriptors(
    const IrSystem& system,
    const std::string& external_system_call_adapters
);
std::string generate_go_api_descriptors(const IrSystem& system);
std::string generate_go_worker_descriptors(const IrSystem& system);
std::string generate_go_worker_descriptor_module(const IrWorker& worker);
std::string generate_go_policy_descriptors(const IrSystem& system);
std::string generate_go_shape_descriptors(const IrSystem& system);
std::string generate_go_observability_descriptors(const IrSystem& system);
std::string generate_go_runtime_descriptors(const IrSystem& system);
std::string generate_go_workflow_descriptor(const IrWorkflow& workflow);
std::string generate_go_workflow_descriptor_umbrella(const IrSystem& system);
std::string generate_go_observability_registration(const IrSystem& system);
std::string generate_go_runtime_registration(
    const IrSystem& system,
    const TemplatePackage& templates
);
std::string generate_go_runtime_registration_domain(
    const TemplatePackage& templates,
    std::string_view name
);

} // namespace statespec
