#pragma once

#include "statespec/ir.hpp"

namespace statespec
{

std::vector<IrField> lower_fields(const std::vector<SemanticField>& fields);

std::optional<std::string> reference_name(const std::optional<SemanticReference>& reference);

std::vector<std::string> reference_names(const std::vector<SemanticReference>& references);

} // namespace statespec
