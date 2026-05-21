#include "openapi_schema.hpp"

#include "openapi_json.hpp"
#include "string_utils.hpp"
#include "type_syntax.hpp"

#include <ostream>
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

} // namespace

std::string openapi_schema_for_type(
    const IrSystem& system,
    const std::string& raw_type
)
{
    const auto type = strip_optional_type(raw_type);
    const auto lowered = lower_ascii(type);

    if (find_shape(system, type) != nullptr)
    {
        return "{ \"$ref\": \"#/components/schemas/" + openapi_json_escape(type) + "\" }";
    }
    if (starts_with(type, "list<") && ends_with(type, ">"))
    {
        const auto item_type = trim_wrapped_type(type, "list<");
        return "{ \"type\": \"array\", \"items\": " + openapi_schema_for_type(system, item_type) +
               " }";
    }
    if (ends_with(type, "[]"))
    {
        const auto item_type = type.substr(0, type.size() - 2);
        return "{ \"type\": \"array\", \"items\": " + openapi_schema_for_type(system, item_type) +
               " }";
    }
    if (starts_with(type, "map<") || lowered == "json" || lowered == "object")
    {
        return "{ \"type\": \"object\", \"additionalProperties\": true }";
    }
    if (lowered == "bool" || lowered == "boolean")
    {
        return "{ \"type\": \"boolean\" }";
    }
    if (lowered == "int" || lowered == "int32" || lowered == "int64" || lowered == "long")
    {
        return "{ \"type\": \"integer\" }";
    }
    if (lowered == "float" || lowered == "double" || lowered == "decimal")
    {
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

    return "{ \"type\": \"string\" }";
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
