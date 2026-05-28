#include "generator_entity_index_helpers.hpp"

#include "identifier_case.hpp"

#include <algorithm>
#include <cstddef>
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

const IrIndex* select_entity_list_index(
    const IrEntity& entity,
    const std::string& api_path
)
{
    const IrIndex* selected = nullptr;
    std::size_t selected_prefix_fields = 0;
    for (const auto& index : entity.indexes)
    {
        std::size_t prefix_fields = 0;
        for (const auto& field_name : index.fields)
        {
            if (api_path.find("{" + field_name + "}") == std::string::npos)
            {
                break;
            }
            ++prefix_fields;
        }
        if (prefix_fields > selected_prefix_fields)
        {
            selected = &index;
            selected_prefix_fields = prefix_fields;
        }
    }
    if (selected != nullptr)
    {
        return selected;
    }
    return entity.indexes.empty() ? nullptr : &entity.indexes.front();
}

const IrIndex* select_entity_list_index_for_selector(
    const IrEntity& entity,
    const std::vector<std::string>& selector
)
{
    if (selector.empty())
    {
        return nullptr;
    }
    for (const auto& index : entity.indexes)
    {
        if (selector.size() <= index.fields.size() &&
            std::equal(selector.begin(), selector.end(), index.fields.begin()))
        {
            return &index;
        }
    }
    return nullptr;
}

} // namespace statespec
