#pragma once

#include "statespec/ir.hpp"

#include <iosfwd>
#include <string>

namespace statespec
{

std::string openapi_api_path(const IrApi& api);

std::string openapi_api_method(const IrApi& api);

bool openapi_has_api_operation(
    const IrSystem& system,
    const std::string& method,
    const std::string& path
);

void write_openapi_operation(
    std::ostream& out,
    const IrSystem& system,
    const IrApi& api
);

} // namespace statespec
