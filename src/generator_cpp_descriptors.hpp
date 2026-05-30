#pragma once

#include "statespec/generator_bindings.hpp"
#include "statespec/template_renderer.hpp"

#include <string>

namespace statespec
{

std::string generate_system_descriptors_header(
    const IrSystem& system,
    const TemplatePackage& templates
);

std::string generate_workflow_step_handler_keys(const IrSystem& system);

std::string generate_workflow_step_handler_methods(const IrSystem& system);

std::string generate_default_workflow_step_handler_methods(const IrSystem& system);

std::string generate_workflow_step_dispatch_cases(const IrSystem& system);

std::string generate_workflow_step_next_cases(const IrSystem& system);

std::string generate_api_operation_handler_methods(const IrSystem& system);

std::string generate_business_api_operation_handler_methods(const IrSystem& system);

std::string generate_api_operation_dispatch_cases(const IrSystem& system);

std::string generate_api_handler_lookup_entries(const IrSystem& system);

std::string generate_api_operation_default_handler_methods(const IrSystem& system);

std::string generate_api_operation_default_handler_domain_methods(const IrSystem& system);

std::string generate_api_codecs(const IrSystem& system);

std::string generate_api_codec_helpers();

std::string generate_api_codec_operations(const IrSystem& system);

} // namespace statespec
