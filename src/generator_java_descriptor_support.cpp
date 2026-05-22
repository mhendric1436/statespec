#include "generator_java_descriptor_support.hpp"

#include "type_syntax.hpp"

#include <cctype>
#include <sstream>

namespace statespec
{

std::string java_string(const std::string& value)
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

std::string java_field_type_expr(FieldDescriptorType type)
{
    switch (type)
    {
    case FieldDescriptorType::String:
        return "FieldType.STRING";
    case FieldDescriptorType::Bool:
        return "FieldType.BOOL";
    case FieldDescriptorType::Int:
        return "FieldType.INT";
    case FieldDescriptorType::Int32:
        return "FieldType.INT32";
    case FieldDescriptorType::Int64:
        return "FieldType.INT64";
    case FieldDescriptorType::Long:
        return "FieldType.LONG";
    case FieldDescriptorType::Double:
        return "FieldType.DOUBLE";
    case FieldDescriptorType::Decimal:
        return "FieldType.DECIMAL";
    case FieldDescriptorType::Json:
        return "FieldType.JSON";
    case FieldDescriptorType::Timestamp:
        return "FieldType.TIMESTAMP";
    case FieldDescriptorType::Duration:
        return "FieldType.DURATION";
    case FieldDescriptorType::Uuid:
        return "FieldType.UUID";
    case FieldDescriptorType::Named:
        return "FieldType.NAMED";
    case FieldDescriptorType::List:
        return "FieldType.LIST";
    case FieldDescriptorType::Set:
        return "FieldType.SET";
    case FieldDescriptorType::Map:
        return "FieldType.MAP";
    case FieldDescriptorType::Optional:
        return "FieldType.OPTIONAL";
    case FieldDescriptorType::Reference:
        return "FieldType.REFERENCE";
    }
    return "FieldType.NAMED";
}

bool is_required_descriptor_field(const std::string& type)
{
    return classify_field_descriptor_type(type) != FieldDescriptorType::Optional;
}

} // namespace

std::string java_field_descriptor_expr(const IrField& field)
{
    return "new FieldDescriptor(" + java_string(field.name) + ", " +
           java_field_type_expr(classify_field_descriptor_type(field.type)) + ", " +
           java_string(field.type) + ", " +
           (is_required_descriptor_field(field.type) ? "true" : "false") + ")";
}

std::string java_shape_type(const std::string& type)
{
    const auto optional = is_optional_type(type);
    const auto base = strip_optional_type(type);
    std::string mapped = "String";
    if (base == "bool")
    {
        mapped = "Boolean";
    }
    else if (base == "int" || base == "int32")
    {
        mapped = "Integer";
    }
    else if (base == "int64" || base == "long")
    {
        mapped = "Long";
    }
    else if (base == "double" || base == "decimal")
    {
        mapped = "Double";
    }
    else if (base == "json")
    {
        mapped = "Json";
    }

    return optional ? "Optional<" + mapped + ">" : mapped;
}

long long parse_java_duration_seconds(const std::optional<std::string>& value)
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

std::string java_optional_string_expr(const std::optional<std::string>& value)
{
    return value.has_value() ? "Optional.of(" + java_string(*value) + ")" : "Optional.empty()";
}

std::string optional_duration_expr(const std::optional<std::string>& value)
{
    return value.has_value() ? "Optional.of(Duration.ofSeconds(" +
                                   std::to_string(parse_java_duration_seconds(value)) + "L))"
                             : "Optional.empty()";
}

const IrApi* find_java_api(
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

std::string java_list_expr(const std::vector<std::string>& values)
{
    std::ostringstream out;
    out << "List.of(";
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << java_string(values[i]);
    }
    out << ")";
    return out.str();
}

} // namespace statespec
