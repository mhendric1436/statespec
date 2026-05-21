#pragma once

#include "statespec/generator_bindings.hpp"

#include <string>

namespace statespec
{

std::string generate_descriptors_go(const IrSystem& system);

std::string generate_workflow_step_handler_keys_go(const IrSystem& system);

} // namespace statespec
