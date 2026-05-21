#pragma once

#include "statespec/ir_core.hpp"

namespace statespec
{

struct IrLog
{
    std::string name;
    std::string level;
    std::string event_name;
    std::vector<IrField> fields;
};

struct IrMetric
{
    std::string name;
    std::string kind;
    std::string backend_name;
    std::string unit;
    std::vector<IrField> labels;
};

} // namespace statespec
