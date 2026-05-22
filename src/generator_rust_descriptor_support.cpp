#include "generator_rust_descriptor_support.hpp"

#include "identifier_case.hpp"
#include "type_syntax.hpp"

#include <cctype>
#include <sstream>

namespace statespec
{

std::string rust_string(const std::string& value)
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

std::string rust_entity_state_constant_name(
    const std::string& entity_name,
    const std::string& state_name
)
{
    return upper_snake_identifier(entity_name + "_status_" + state_name);
}

namespace
{

std::string rust_field_type_expr(FieldDescriptorType type)
{
    switch (type)
    {
    case FieldDescriptorType::String:
        return "FieldType::String";
    case FieldDescriptorType::Bool:
        return "FieldType::Bool";
    case FieldDescriptorType::Int:
        return "FieldType::Int";
    case FieldDescriptorType::Int32:
        return "FieldType::Int32";
    case FieldDescriptorType::Int64:
        return "FieldType::Int64";
    case FieldDescriptorType::Long:
        return "FieldType::Long";
    case FieldDescriptorType::Double:
        return "FieldType::Double";
    case FieldDescriptorType::Decimal:
        return "FieldType::Decimal";
    case FieldDescriptorType::Json:
        return "FieldType::Json";
    case FieldDescriptorType::Timestamp:
        return "FieldType::Timestamp";
    case FieldDescriptorType::Duration:
        return "FieldType::Duration";
    case FieldDescriptorType::Uuid:
        return "FieldType::Uuid";
    case FieldDescriptorType::Named:
        return "FieldType::Named";
    case FieldDescriptorType::List:
        return "FieldType::List";
    case FieldDescriptorType::Set:
        return "FieldType::Set";
    case FieldDescriptorType::Map:
        return "FieldType::Map";
    case FieldDescriptorType::Optional:
        return "FieldType::Optional";
    case FieldDescriptorType::Reference:
        return "FieldType::Reference";
    }
    return "FieldType::Named";
}

bool is_required_descriptor_field(const std::string& type)
{
    return classify_field_descriptor_type(type) != FieldDescriptorType::Optional;
}

} // namespace

std::string rust_field_descriptor_expr(const IrField& field)
{
    return "FieldDescriptor { name: " + rust_string(field.name) + ".to_string(), field_type: " +
           rust_field_type_expr(classify_field_descriptor_type(field.type)) +
           ", type_name: " + rust_string(field.type) + ".to_string(), required: " +
           (is_required_descriptor_field(field.type) ? "true" : "false") + " }";
}

std::string rust_shape_type(const std::string& type)
{
    const auto optional = is_optional_type(type);
    const auto base = strip_optional_type(type);
    std::string mapped = "String";
    if (base == "bool")
    {
        mapped = "bool";
    }
    else if (base == "int" || base == "int32")
    {
        mapped = "i32";
    }
    else if (base == "int64" || base == "long")
    {
        mapped = "i64";
    }
    else if (base == "double" || base == "decimal")
    {
        mapped = "f64";
    }
    else if (base == "json")
    {
        mapped = "Json";
    }

    return optional ? "Option<" + mapped + ">" : mapped;
}

long long parse_rust_duration_seconds(const std::optional<std::string>& value)
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

std::string rust_optional_string_expr(const std::optional<std::string>& value)
{
    return value.has_value() ? "Some(" + rust_string(*value) + ".to_string())" : "None";
}

std::string rust_optional_duration_expr(const std::optional<std::string>& value)
{
    return value.has_value() ? "Some(Duration::from_secs(" +
                                   std::to_string(parse_rust_duration_seconds(value)) + "))"
                             : "None";
}

const IrApi* find_rust_api(
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
