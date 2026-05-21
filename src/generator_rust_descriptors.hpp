#pragma once

#include "statespec/generator_bindings.hpp"

#include <string>

namespace statespec
{

std::string generate_descriptors_rs(const IrSystem& system);

std::string generate_workflow_step_handler_keys_rs(const IrSystem& system);

} // namespace statespec
