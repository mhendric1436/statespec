#pragma once

#include "statespec/generator_bindings.hpp"
#include "statespec/template_renderer.hpp"

#include <string>

namespace statespec
{

std::string generate_descriptors_rs(
    const IrSystem& system,
    const TemplatePackage& templates
);

std::string generate_workflow_step_handler_keys_rs(const IrSystem& system);

std::string generate_workflow_step_handler_methods_rs(const IrSystem& system);

std::string generate_default_workflow_step_handler_methods_rs(const IrSystem& system);

std::string generate_workflow_step_dispatch_cases_rs(const IrSystem& system);

std::string generate_workflow_step_next_cases_rs(const IrSystem& system);

std::string generate_api_operation_handler_methods_rs(const IrSystem& system);

std::string generate_business_api_operation_handler_methods_rs(const IrSystem& system);

std::string generate_api_operation_dispatch_cases_rs(const IrSystem& system);

std::string generate_api_handler_lookup_entries_rs(const IrSystem& system);

std::string generate_api_operation_default_handler_methods_rs(const IrSystem& system);

std::string generate_api_operation_default_handler_domain_methods_rs(const IrSystem& system);

std::string generate_api_codecs_rs(const IrSystem& system);

std::string generate_api_codec_helpers_rs();

std::string generate_api_codec_operations_rs(const IrSystem& system);

} // namespace statespec
