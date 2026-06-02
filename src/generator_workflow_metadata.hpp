#pragma once

#include "statespec/ir.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

struct GeneratedWorkflowSyntheticPhase
{
    std::string step_name;
    std::optional<std::string> next_step;
};

std::string workflow_descriptor_metadata_json(const IrWorkflow& workflow);
std::vector<GeneratedWorkflowSyntheticPhase>
workflow_synthetic_child_phases(const IrWorkflow& workflow);

} // namespace statespec
