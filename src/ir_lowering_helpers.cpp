#include "ir_lowering_helpers.hpp"

namespace statespec
{

std::vector<IrField> lower_fields(const std::vector<SemanticField>& fields)
{
    std::vector<IrField> lowered;
    for (const auto& field : fields)
    {
        lowered.push_back(IrField{field.name, field.type});
    }
    return lowered;
}

std::optional<std::string> reference_name(const std::optional<SemanticReference>& reference)
{
    if (!reference.has_value())
    {
        return std::nullopt;
    }
    return reference->name;
}

std::vector<std::string> reference_names(const std::vector<SemanticReference>& references)
{
    std::vector<std::string> names;
    for (const auto& reference : references)
    {
        names.push_back(reference.name);
    }
    return names;
}

} // namespace statespec
