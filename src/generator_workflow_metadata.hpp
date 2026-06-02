#pragma once

#include "statespec/ir.hpp"

#include <string>

namespace statespec
{

std::string workflow_descriptor_metadata_json(const IrWorkflow& workflow);

} // namespace statespec
