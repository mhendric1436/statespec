#include "generator_go_descriptor_support.hpp"

#include "identifier_case.hpp"
#include "type_syntax.hpp"

#include <cctype>
#include <sstream>

namespace statespec
{

std::string go_string(const std::string& value)
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

std::string go_entity_state_constant_name(
    const std::string& entity_name,
    const std::string& state_name
)
{
    return pascal_identifier(entity_name) + "Status" + pascal_identifier(state_name);
}

namespace
{

std::string go_field_type_expr(FieldDescriptorType type)
{
    switch (type)
    {
    case FieldDescriptorType::String:
        return "FieldTypeString";
    case FieldDescriptorType::Bool:
        return "FieldTypeBool";
    case FieldDescriptorType::Int:
        return "FieldTypeInt";
    case FieldDescriptorType::Int32:
        return "FieldTypeInt32";
    case FieldDescriptorType::Int64:
        return "FieldTypeInt64";
    case FieldDescriptorType::Long:
        return "FieldTypeLong";
    case FieldDescriptorType::Double:
        return "FieldTypeDouble";
    case FieldDescriptorType::Decimal:
        return "FieldTypeDecimal";
    case FieldDescriptorType::Json:
        return "FieldTypeJSON";
    case FieldDescriptorType::Timestamp:
        return "FieldTypeTimestamp";
    case FieldDescriptorType::Duration:
        return "FieldTypeDuration";
    case FieldDescriptorType::Uuid:
        return "FieldTypeUUID";
    case FieldDescriptorType::Named:
        return "FieldTypeNamed";
    case FieldDescriptorType::List:
        return "FieldTypeList";
    case FieldDescriptorType::Set:
        return "FieldTypeSet";
    case FieldDescriptorType::Map:
        return "FieldTypeMap";
    case FieldDescriptorType::Optional:
        return "FieldTypeOptional";
    case FieldDescriptorType::Reference:
        return "FieldTypeReference";
    }
    return "FieldTypeNamed";
}

bool is_required_descriptor_field(const std::string& type)
{
    return classify_field_descriptor_type(type) != FieldDescriptorType::Optional;
}

} // namespace

std::string go_field_descriptor_expr(const IrField& field)
{
    return "{Name: " + go_string(field.name) +
           ", Type: " + go_field_type_expr(classify_field_descriptor_type(field.type)) +
           ", TypeName: " + go_string(field.type) +
           ", Required: " + (is_required_descriptor_field(field.type) ? "true" : "false") + "}";
}

std::string go_shape_type(const std::string& type)
{
    const auto optional = is_optional_type(type);
    const auto base = strip_optional_type(type);
    std::string mapped = "string";
    if (base == "bool")
    {
        mapped = "bool";
    }
    else if (base == "int" || base == "int32")
    {
        mapped = "int32";
    }
    else if (base == "int64" || base == "long")
    {
        mapped = "int64";
    }
    else if (base == "double" || base == "decimal")
    {
        mapped = "float64";
    }
    else if (base == "json")
    {
        mapped = "JSON";
    }

    return optional ? "*" + mapped : mapped;
}

long long parse_go_duration_seconds(const std::optional<std::string>& value)
{
    if (!value.has_value() || value->empty())
    {
        return 0;
    }
    const auto& text = *value;
    if (text.size() < 3 || text[0] != 'P' || text[1] != 'T')
    {
        return 0;
    }

    long long total = 0;
    long long current = 0;
    for (std::size_t i = 2; i < text.size(); ++i)
    {
        const auto ch = text[i];
        if (std::isdigit(static_cast<unsigned char>(ch)))
        {
            current = current * 10 + static_cast<long long>(ch - '0');
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

std::string string_ptr_expr(const std::optional<std::string>& value)
{
    return value.has_value() ? "stringPtr(" + go_string(*value) + ")" : "nil";
}

std::string duration_ptr_expr(const std::optional<std::string>& value)
{
    if (!value.has_value())
    {
        return "nil";
    }
    return "durationPtr(" + std::to_string(parse_go_duration_seconds(value)) + " * time.Second)";
}

const IrApi* find_go_api(
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
