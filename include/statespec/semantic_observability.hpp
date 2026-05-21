#pragma once

#include "statespec/semantic_core.hpp"

namespace statespec
{

struct SemanticLog
{
    std::string name;
    std::string level;
    std::string event_name;
    std::vector<SemanticField> fields;
};

struct SemanticMetric
{
    std::string name;
    std::string kind;
    std::string backend_name;
    std::string unit;
    std::vector<SemanticField> labels;
};

} // namespace statespec
