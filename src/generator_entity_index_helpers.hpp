#pragma once

#include <string>

namespace statespec
{

std::string entity_index_repository_method_name(const std::string& index_name);
std::string go_entity_index_repository_method_name(const std::string& index_name);
std::string rust_entity_index_repository_method_name(const std::string& index_name);

} // namespace statespec
