#pragma once

#include "statespec/ir.hpp"

#include <string>

namespace statespec
{

std::string entity_index_repository_method_name(const std::string& index_name);
std::string go_entity_index_repository_method_name(const std::string& index_name);
std::string rust_entity_index_repository_method_name(const std::string& index_name);
const IrIndex* select_entity_list_index(
    const IrEntity& entity,
    const std::string& api_path
);
const IrIndex* select_entity_list_index_for_selector(
    const IrEntity& entity,
    const std::vector<std::string>& selector
);

} // namespace statespec
