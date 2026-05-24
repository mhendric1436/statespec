#include "generator_entity_index_helpers.hpp"

#include "identifier_case.hpp"

#include <string_view>

namespace statespec
{
namespace
{

std::string index_name_suffix(const std::string& index_name)
{
    constexpr std::string_view by_prefix = "by_";
    if (index_name.size() > by_prefix.size() &&
        index_name.compare(0, by_prefix.size(), by_prefix) == 0)
    {
        return index_name.substr(by_prefix.size());
    }
    return index_name;
}

} // namespace

std::string entity_index_repository_method_name(const std::string& index_name)
{
    return "listBy" + pascal_identifier(index_name_suffix(index_name), "Index") + "Tx";
}

std::string go_entity_index_repository_method_name(const std::string& index_name)
{
    return "ListBy" + pascal_identifier(index_name_suffix(index_name), "Index") + "Tx";
}

std::string rust_entity_index_repository_method_name(const std::string& index_name)
{
    return "list_by_" + snake_identifier(index_name_suffix(index_name), "index") + "_tx";
}

} // namespace statespec
