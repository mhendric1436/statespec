#pragma once

#include "statespec/generator_bindings.hpp"

#include <string>

namespace statespec
{

std::string generate_system_descriptors_header(const IrSystem& system);

std::string generate_workflow_step_handler_keys(const IrSystem& system);

std::string generate_workflow_step_handler_methods(const IrSystem& system);

std::string generate_workflow_step_dispatch_cases(const IrSystem& system);

std::string generate_workflow_step_next_cases(const IrSystem& system);

std::string generate_api_operation_handler_methods(const IrSystem& system);

std::string generate_api_operation_dispatch_cases(const IrSystem& system);

std::string generate_api_operation_default_handler_methods(const IrSystem& system);

} // namespace statespec
