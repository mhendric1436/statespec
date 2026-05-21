#pragma once

#include "statespec/ir.hpp"

#include <string>

namespace statespec
{

std::string render_openapi(const IrSystem& system);

} // namespace statespec
