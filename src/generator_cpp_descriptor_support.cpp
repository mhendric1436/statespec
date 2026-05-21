#include "generator_cpp_descriptor_support.hpp"

#include "type_syntax.hpp"

#include <cctype>
#include <sstream>

namespace statespec
{

std::string cpp_string(const std::string& value)
{
    std::ostringstream out;
    out << '"';
    for (const auto ch : value)
    {
        switch (ch)
        {
        case '"':
            out << "\\\"";
            break;
        case '\\':
            out << "\\\\";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            out << ch;
            break;
        }
    }
    out << '"';
    return out.str();
}

namespace
{

std::string cpp_field_type_expr(FieldDescriptorType type)
{
    switch (type)
    {
    case FieldDescriptorType::String:
        return "statespec::backend::FieldType::String";
    case FieldDescriptorType::Bool:
        return "statespec::backend::FieldType::Bool";
    case FieldDescriptorType::Int:
        return "statespec::backend::FieldType::Int";
    case FieldDescriptorType::Int32:
        return "statespec::backend::FieldType::Int32";
    case FieldDescriptorType::Int64:
        return "statespec::backend::FieldType::Int64";
    case FieldDescriptorType::Long:
        return "statespec::backend::FieldType::Long";
    case FieldDescriptorType::Double:
        return "statespec::backend::FieldType::Double";
    case FieldDescriptorType::Decimal:
        return "statespec::backend::FieldType::Decimal";
    case FieldDescriptorType::Json:
        return "statespec::backend::FieldType::Json";
    case FieldDescriptorType::Timestamp:
        return "statespec::backend::FieldType::Timestamp";
    case FieldDescriptorType::Duration:
        return "statespec::backend::FieldType::Duration";
    case FieldDescriptorType::Uuid:
        return "statespec::backend::FieldType::Uuid";
    case FieldDescriptorType::Named:
        return "statespec::backend::FieldType::Named";
    case FieldDescriptorType::List:
        return "statespec::backend::FieldType::List";
    case FieldDescriptorType::Set:
        return "statespec::backend::FieldType::Set";
    case FieldDescriptorType::Map:
        return "statespec::backend::FieldType::Map";
    case FieldDescriptorType::Optional:
        return "statespec::backend::FieldType::Optional";
    case FieldDescriptorType::Reference:
        return "statespec::backend::FieldType::Reference";
    }
    return "statespec::backend::FieldType::Named";
}

bool is_required_descriptor_field(const std::string& type)
{
    return classify_field_descriptor_type(type) != FieldDescriptorType::Optional;
}

} // namespace

std::string cpp_field_descriptor_expr(const IrField& field)
{
    return "statespec::backend::FieldDescriptor{" + cpp_string(field.name) + ", " +
           cpp_field_type_expr(classify_field_descriptor_type(field.type)) + ", " +
           cpp_string(field.type) + ", " +
           (is_required_descriptor_field(field.type) ? "true" : "false") + "}";
}

std::string cpp_shape_type(const std::string& type)
{
    const auto optional = is_optional_type(type);
    const auto base = strip_optional_type(type);
    std::string mapped = "std::string";
    if (base == "bool")
    {
        mapped = "bool";
    }
    else if (base == "int" || base == "int32")
    {
        mapped = "std::int32_t";
    }
    else if (base == "int64" || base == "long")
    {
        mapped = "std::int64_t";
    }
    else if (base == "double" || base == "decimal")
    {
        mapped = "double";
    }

    return optional ? "std::optional<" + mapped + ">" : mapped;
}

std::int64_t parse_duration_seconds(const std::optional<std::string>& value)
{
    if (!value.has_value() || value->empty())
    {
        return 0;
    }

    const auto& text = *value;
    if (text.size() >= 3 && text[0] == 'P' && text[1] == 'T')
    {
        std::int64_t total = 0;
        std::int64_t current = 0;
        for (std::size_t i = 2; i < text.size(); ++i)
        {
            const auto ch = text[i];
            if (std::isdigit(static_cast<unsigned char>(ch)))
            {
                current = current * 10 + static_cast<std::int64_t>(ch - '0');
            }
            else
            {
                if (ch == 'H')
                {
                    total += current * 3600;
                }
                else if (ch == 'M')
                {
                    total += current * 60;
                }
                else if (ch == 'S')
                {
                    total += current;
                }
                current = 0;
            }
        }
        return total;
    }

    return 0;
}

std::string optional_string_expr(const std::optional<std::string>& value)
{
    if (!value.has_value())
    {
        return "std::nullopt";
    }
    return "std::optional<std::string>{" + cpp_string(*value) + "}";
}

const IrApi* find_api(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& api : system.apis)
    {
        if (api.name == name)
        {
            return &api;
        }
    }
    return nullptr;
}

} // namespace statespec
