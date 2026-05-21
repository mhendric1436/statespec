#pragma once

#include <string>

namespace statespec
{

std::string openapi_json_escape(const std::string& value);

std::string openapi_quoted(const std::string& value);

} // namespace statespec
