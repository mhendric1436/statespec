#pragma once

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

struct IrField
{
    std::string name;
    std::string type;
};

struct IrTenantScope
{
    std::string field_name;
};

struct IrSystemTenant
{
    std::string source;
    std::string config_key;
};

} // namespace statespec
