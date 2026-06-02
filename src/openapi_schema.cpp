#include "openapi_schema.hpp"

#include "openapi_json.hpp"
#include "string_utils.hpp"
#include "type_syntax.hpp"

#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

namespace statespec
{

namespace
{

const IrShape* find_shape(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& shape : system.shapes)
    {
        if (shape.name == name)
        {
            return &shape;
        }
    }
    return nullptr;
}

const IrValue* find_value(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& value : system.values)
    {
        if (value.name == name)
        {
            return &value;
        }
    }
    return nullptr;
}

const IrEnum* find_enum(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& enum_decl : system.enums)
    {
        if (enum_decl.name == name)
        {
            return &enum_decl;
        }
    }
    return nullptr;
}

std::string add_openapi_schema_property(
    std::string schema,
    const std::string& property
)
{
    const auto close = schema.rfind('}');
    if (close == std::string::npos)
    {
        return schema;
    }
    schema.insert(close, ", " + property + " ");
    return schema;
}

std::vector<std::string> split_top_level_csv(std::string_view value)
{
    std::vector<std::string> values;
    std::size_t begin = 0;
    int depth = 0;
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '<')
        {
            ++depth;
        }
        else if (value[i] == '>')
        {
            --depth;
        }
        else if (value[i] == ',' && depth == 0)
        {
            values.push_back(trim_ascii_copy(value.substr(begin, i - begin)));
            begin = i + 1;
        }
    }
    values.push_back(trim_ascii_copy(value.substr(begin)));
    return values;
}

std::optional<std::string> generic_body(
    const std::string& type,
    std::string_view generic_name
)
{
    const auto lowered = lower_ascii(type);
    const std::string prefix = std::string{generic_name} + "<";
    if (!starts_with(lowered, prefix) || !ends_with(type, ">"))
    {
        return std::nullopt;
    }
    return type.substr(prefix.size(), type.size() - prefix.size() - 1);
}

std::string openapi_ref_schema(const std::string& name)
{
    return "{ \"$ref\": \"#/components/schemas/" + openapi_json_escape(name) + "\" }";
}

} // namespace

std::string openapi_schema_for_type(
    const IrSystem& system,
    const std::string& raw_type
)
{
    const auto type = strip_optional_type(raw_type);
    const auto lowered = lower_ascii(type);

    if (find_value(system, type) != nullptr || find_enum(system, type) != nullptr ||
        find_shape(system, type) != nullptr)
    {
        return openapi_ref_schema(type);
    }
    if (const auto item_type = generic_body(type, "list"))
    {
        return "{ \"type\": \"array\", \"items\": " + openapi_schema_for_type(system, *item_type) +
               " }";
    }
    if (const auto item_type = generic_body(type, "set"))
    {
        return "{ \"type\": \"array\", \"uniqueItems\": true, \"items\": " +
               openapi_schema_for_type(system, *item_type) + " }";
    }
    if (ends_with(type, "[]"))
    {
        const auto item_type = type.substr(0, type.size() - 2);
        return "{ \"type\": \"array\", \"items\": " + openapi_schema_for_type(system, item_type) +
               " }";
    }
    if (const auto map_body = generic_body(type, "map"))
    {
        const auto args = split_top_level_csv(*map_body);
        if (args.size() == 2)
        {
            return "{ \"type\": \"object\", \"additionalProperties\": " +
                   openapi_schema_for_type(system, args[1]) + " }";
        }
        return "{ \"type\": \"object\", \"additionalProperties\": true }";
    }
    if (lowered == "json" || lowered == "object")
    {
        return "{ \"type\": \"object\", \"additionalProperties\": true }";
    }
    if (lowered == "bool" || lowered == "boolean")
    {
        return "{ \"type\": \"boolean\" }";
    }
    if (lowered == "int" || lowered == "int32" || lowered == "int64" || lowered == "long")
    {
        if (lowered == "int32" || lowered == "int")
        {
            return "{ \"type\": \"integer\", \"format\": \"int32\" }";
        }
        return "{ \"type\": \"integer\", \"format\": \"int64\" }";
    }
    if (lowered == "float" || lowered == "double" || lowered == "decimal")
    {
        if (lowered == "float")
        {
            return "{ \"type\": \"number\", \"format\": \"float\" }";
        }
        if (lowered == "double")
        {
            return "{ \"type\": \"number\", \"format\": \"double\" }";
        }
        return "{ \"type\": \"number\" }";
    }
    if (lowered == "uuid")
    {
        return "{ \"type\": \"string\", \"format\": \"uuid\" }";
    }
    if (lowered == "timestamp" || lowered == "datetime")
    {
        return "{ \"type\": \"string\", \"format\": \"date-time\" }";
    }
    if (lowered == "date")
    {
        return "{ \"type\": \"string\", \"format\": \"date\" }";
    }
    if (lowered == "duration")
    {
        return "{ \"type\": \"string\", \"format\": \"duration\" }";
    }

    return "{ \"type\": \"string\" }";
}

void write_openapi_value_schema(
    std::ostream& out,
    const IrSystem& system,
    const IrValue& value
)
{
    auto schema = openapi_schema_for_type(system, value.type);
    if (value.constraint.has_value() && !value.constraint->empty())
    {
        schema = add_openapi_schema_property(
            std::move(schema), "\"x-statespec-constraint\": " + openapi_quoted(*value.constraint)
        );
    }
    out << "      " << openapi_quoted(value.name) << ": " << schema;
}

void write_openapi_enum_schema(
    std::ostream& out,
    const IrEnum& enum_decl
)
{
    out << "      " << openapi_quoted(enum_decl.name) << ": {\n";
    out << "        \"type\": \"string\",\n";
    out << "        \"enum\": [";
    for (std::size_t i = 0; i < enum_decl.members.size(); ++i)
    {
        const auto& member = enum_decl.members[i];
        out << (i == 0 ? "" : ", ") << openapi_quoted(member.value.value_or(member.name));
    }
    out << "]\n";
    out << "      }";
}

void write_openapi_shape_schema(
    std::ostream& out,
    const IrSystem& system,
    const IrShape& shape
)
{
    out << "      " << openapi_quoted(shape.name) << ": {\n";
    out << "        \"type\": \"object\",\n";
    out << "        \"properties\": {\n";
    for (std::size_t i = 0; i < shape.fields.size(); ++i)
    {
        const auto& field = shape.fields[i];
        out << "          " << openapi_quoted(field.name) << ": "
            << openapi_schema_for_type(system, field.type);
        out << (i + 1 == shape.fields.size() ? "\n" : ",\n");
    }
    out << "        }";

    std::vector<std::string> required;
    for (const auto& field : shape.fields)
    {
        if (!is_optional_type(field.type))
        {
            required.push_back(field.name);
        }
    }
    if (!required.empty())
    {
        out << ",\n";
        out << "        \"required\": [";
        for (std::size_t i = 0; i < required.size(); ++i)
        {
            out << (i == 0 ? "" : ", ") << openapi_quoted(required[i]);
        }
        out << "]\n";
        out << "      }";
    }
    else
    {
        out << "\n";
        out << "      }";
    }
}

void write_openapi_media_schema(
    std::ostream& out,
    const IrSystem& system,
    const std::string& type,
    const std::string& indent
)
{
    out << indent << "\"content\": {\n";
    out << indent << "  \"application/json\": {\n";
    out << indent << "    \"schema\": " << openapi_schema_for_type(system, type) << "\n";
    out << indent << "  }\n";
    out << indent << "}";
}

} // namespace statespec
