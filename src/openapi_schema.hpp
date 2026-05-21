#pragma once

#include "statespec/ir.hpp"

#include <iosfwd>
#include <string>

namespace statespec
{

std::string openapi_schema_for_type(
    const IrSystem& system,
    const std::string& raw_type
);

void write_openapi_shape_schema(
    std::ostream& out,
    const IrSystem& system,
    const IrShape& shape
);

void write_openapi_media_schema(
    std::ostream& out,
    const IrSystem& system,
    const std::string& type,
    const std::string& indent
);

} // namespace statespec
