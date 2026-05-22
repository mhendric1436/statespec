#pragma once

#include "statespec/generator_bindings.hpp"

#include <string>

namespace statespec
{

std::string generate_descriptors_rs(const IrSystem& system);

std::string generate_workflow_step_handler_keys_rs(const IrSystem& system);

std::string generate_api_operation_handler_methods_rs(const IrSystem& system);

std::string generate_api_operation_dispatch_cases_rs(const IrSystem& system);

std::string generate_api_operation_default_handler_methods_rs(const IrSystem& system);

} // namespace statespec
