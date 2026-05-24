#pragma once

#include "statespec/generator_bindings.hpp"
#include "statespec/template_renderer.hpp"

#include <string>

namespace statespec
{

std::string generate_descriptors_go(
    const IrSystem& system,
    const TemplatePackage& templates
);

std::string generate_workflow_step_handler_keys_go(const IrSystem& system);

std::string generate_workflow_step_handler_methods_go(const IrSystem& system);

std::string generate_default_workflow_step_handler_methods_go(const IrSystem& system);

std::string generate_workflow_step_handler_imports_go(const IrSystem& system);

std::string generate_workflow_step_dispatch_cases_go(const IrSystem& system);

std::string generate_workflow_step_next_cases_go(const IrSystem& system);

std::string generate_api_operation_handler_methods_go(const IrSystem& system);

std::string generate_api_operation_dispatch_cases_go(const IrSystem& system);

std::string generate_api_operation_default_handler_methods_go(const IrSystem& system);

std::string generate_api_codecs_go(const IrSystem& system);

} // namespace statespec
