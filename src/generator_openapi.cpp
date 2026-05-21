#include "statespec/generator_openapi.hpp"

#include "statespec/ir.hpp"

#include "string_utils.hpp"
#include "type_syntax.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace statespec
{
namespace
{

std::string json_escape(const std::string& value)
{
    std::ostringstream out;
    for (const char ch : value)
    {
        switch (ch)
        {
        case '"':
            out << "\\\"";
            break;
        case '\\':
            out << "\\\\";
            break;
        case '\b':
            out << "\\b";
            break;
        case '\f':
            out << "\\f";
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
            const auto uch = static_cast<unsigned char>(ch);
            if (uch < 0x20)
            {
                out << "\\u00";
                constexpr char digits[] = "0123456789abcdef";
                out << digits[(uch >> 4) & 0x0f] << digits[uch & 0x0f];
            }
            else
            {
                out << ch;
            }
            break;
        }
    }
    return out.str();
}

std::string quoted(const std::string& value)
{
    return "\"" + json_escape(value) + "\"";
}

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

const IrEntity* find_entity(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& entity : system.entities)
    {
        if (entity.name == name)
        {
            return &entity;
        }
    }
    return nullptr;
}

bool has_shape(
    const IrSystem& system,
    const std::string& name
)
{
    return find_shape(system, name) != nullptr;
}

std::string api_path(const IrApi& api);
std::string api_method(const IrApi& api);

bool has_api_operation(
    const IrSystem& system,
    const std::string& method,
    const std::string& path
)
{
    const auto normalized_method = lower_ascii(method);
    for (const auto& api : system.apis)
    {
        if (api_method(api) == normalized_method && api_path(api) == path)
        {
            return true;
        }
    }
    return false;
}

bool contains(
    const std::vector<std::string>& values,
    const std::string& value
)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool is_foundational_entity_field(const std::string& name)
{
    return name == "created_at" || name == "updated_at" || name == "status";
}

std::string schema_for_type(
    const IrSystem& system,
    const std::string& raw_type
)
{
    const auto type = strip_optional_type(raw_type);
    const auto lowered = lower_ascii(type);

    if (find_shape(system, type) != nullptr)
    {
        return "{ \"$ref\": \"#/components/schemas/" + json_escape(type) + "\" }";
    }
    if (starts_with(type, "list<") && ends_with(type, ">"))
    {
        const auto item_type = trim_wrapped_type(type, "list<");
        return "{ \"type\": \"array\", \"items\": " + schema_for_type(system, item_type) + " }";
    }
    if (ends_with(type, "[]"))
    {
        const auto item_type = type.substr(0, type.size() - 2);
        return "{ \"type\": \"array\", \"items\": " + schema_for_type(system, item_type) + " }";
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

std::vector<std::string> extract_path_parameters(const std::string& path)
{
    std::vector<std::string> parameters;
    std::size_t offset = 0;
    while (offset < path.size())
    {
        const auto open = path.find('{', offset);
        if (open == std::string::npos)
        {
            break;
        }
        const auto close = path.find('}', open + 1);
        if (close == std::string::npos)
        {
            break;
        }
        if (close > open + 1)
        {
            parameters.push_back(path.substr(open + 1, close - open - 1));
        }
        offset = close + 1;
    }
    return parameters;
}

std::string api_path(const IrApi& api)
{
    if (api.path.has_value() && !api.path->empty())
    {
        return *api.path;
    }
    return "/" + api.name;
}

std::string api_method(const IrApi& api)
{
    if (api.method.has_value() && !api.method->empty())
    {
        return lower_ascii(*api.method);
    }
    return "post";
}

std::string operator_metadata_base_path(const IrExternalSystemMetadata& metadata)
{
    std::string path;
    if (metadata.tenant_field.has_value())
    {
        path += "/v1/tenants/{" + *metadata.tenant_field + "}";
    }
    else
    {
        path += "/v1";
    }
    path += "/operators/external-systems/{external_system_id}/profiles/{";
    path += metadata.profile_field;
    path += "}";
    return path;
}

void append_operator_metadata_shapes(
    IrSystem& system,
    const IrExternalSystemMetadata& metadata
)
{
    const auto* entity = find_entity(system, metadata.entity);
    if (entity == nullptr)
    {
        return;
    }

    const auto request_name = metadata.entity + "Request";
    if (!has_shape(system, request_name))
    {
        IrShape request_shape;
        request_shape.name = request_name;
        for (const auto& field : entity->fields)
        {
            if (!contains(entity->key_fields, field.name) &&
                !is_foundational_entity_field(field.name))
            {
                auto request_field = field;
                if (!contains(metadata.required_fields, field.name) &&
                    !is_optional_type(request_field.type))
                {
                    request_field.type = "optional<" + request_field.type + ">";
                }
                request_shape.fields.push_back(request_field);
            }
        }
        system.shapes.push_back(request_shape);
    }

    const auto response_name = metadata.entity + "Response";
    if (!has_shape(system, response_name))
    {
        IrShape response_shape;
        response_shape.name = response_name;
        response_shape.fields = entity->fields;
        system.shapes.push_back(response_shape);
    }
}

void append_operator_metadata_apis(
    IrSystem& system,
    const IrExternalSystemMetadata& metadata
)
{
    const auto base_path = operator_metadata_base_path(metadata);
    const auto request_name = metadata.entity + "Request";
    const auto response_name = metadata.entity + "Response";

    if (!has_api_operation(system, "PUT", base_path))
    {
        system.apis.push_back(
            IrApi{
                "Upsert" + metadata.entity,
                std::string{"PUT"},
                base_path,
                request_name,
                response_name,
                std::nullopt,
                std::nullopt,
                std::nullopt,
            }
        );
    }
    if (!has_api_operation(system, "GET", base_path))
    {
        system.apis.push_back(
            IrApi{
                "Get" + metadata.entity,
                std::string{"GET"},
                base_path,
                std::nullopt,
                response_name,
                std::nullopt,
                std::nullopt,
                std::nullopt,
            }
        );
    }
    if (!has_api_operation(system, "POST", base_path + "/disable") &&
        !has_api_operation(system, "PATCH", base_path + "/disable"))
    {
        system.apis.push_back(
            IrApi{
                "Disable" + metadata.entity,
                std::string{"POST"},
                base_path + "/disable",
                std::nullopt,
                response_name,
                std::nullopt,
                std::nullopt,
                std::nullopt,
            }
        );
    }
    if (!has_api_operation(system, "DELETE", base_path))
    {
        system.apis.push_back(
            IrApi{
                "Delete" + metadata.entity,
                std::string{"DELETE"},
                base_path,
                std::nullopt,
                response_name,
                std::nullopt,
                std::nullopt,
                std::nullopt,
            }
        );
    }
}

IrSystem with_generated_operator_metadata_openapi(IrSystem system)
{
    for (const auto& external_system : system.external_systems)
    {
        if (external_system.metadata.has_value())
        {
            append_operator_metadata_shapes(system, *external_system.metadata);
            append_operator_metadata_apis(system, *external_system.metadata);
        }
    }
    return system;
}

void write_shape_schema(
    std::ostream& out,
    const IrSystem& system,
    const IrShape& shape
)
{
    out << "      " << quoted(shape.name) << ": {\n";
    out << "        \"type\": \"object\",\n";
    out << "        \"properties\": {\n";
    for (std::size_t i = 0; i < shape.fields.size(); ++i)
    {
        const auto& field = shape.fields[i];
        out << "          " << quoted(field.name) << ": " << schema_for_type(system, field.type);
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
            out << (i == 0 ? "" : ", ") << quoted(required[i]);
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

void write_media_schema(
    std::ostream& out,
    const IrSystem& system,
    const std::string& type,
    const std::string& indent
)
{
    out << indent << "\"content\": {\n";
    out << indent << "  \"application/json\": {\n";
    out << indent << "    \"schema\": " << schema_for_type(system, type) << "\n";
    out << indent << "  }\n";
    out << indent << "}";
}

void write_operation(
    std::ostream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    out << "      \"operationId\": " << quoted(api.name);
    const auto parameters = extract_path_parameters(api_path(api));
    if (!parameters.empty())
    {
        out << ",\n";
        out << "      \"parameters\": [\n";
        for (std::size_t i = 0; i < parameters.size(); ++i)
        {
            out << "        {\n";
            out << "          \"name\": " << quoted(parameters[i]) << ",\n";
            out << "          \"in\": \"path\",\n";
            out << "          \"required\": true,\n";
            out << "          \"schema\": { \"type\": \"string\" }\n";
            out << "        }" << (i + 1 == parameters.size() ? "\n" : ",\n");
        }
        out << "      ]";
    }
    if (api.input.has_value())
    {
        out << ",\n";
        out << "      \"requestBody\": {\n";
        out << "        \"required\": true,\n";
        write_media_schema(out, system, *api.input, "        ");
        out << "\n";
        out << "      }";
    }
    out << ",\n";
    out << "      \"responses\": {\n";
    out << "        \"200\": {\n";
    out << "          \"description\": \"OK\"";
    if (api.output.has_value())
    {
        out << ",\n";
        write_media_schema(out, system, *api.output, "          ");
        out << "\n";
        out << "        }";
    }
    else
    {
        out << "\n";
        out << "        }";
    }
    if (api.error.has_value())
    {
        out << ",\n";
        out << "        \"default\": {\n";
        out << "          \"description\": \"Error\",\n";
        write_media_schema(out, system, *api.error, "          ");
        out << "\n";
        out << "        }\n";
    }
    else
    {
        out << "\n";
    }
    out << "      }\n";
}

std::string render_openapi(const IrSystem& system)
{
    const auto openapi_system = with_generated_operator_metadata_openapi(system);
    std::map<std::string, std::vector<const IrApi*>> apis_by_path;
    for (const auto& api : openapi_system.apis)
    {
        apis_by_path[api_path(api)].push_back(&api);
    }

    std::ostringstream out;
    out << "{\n";
    out << "  \"openapi\": \"3.0.3\",\n";
    out << "  \"info\": {\n";
    out << "    \"title\": " << quoted(openapi_system.name + " API") << ",\n";
    out << "    \"version\": \"0.1.0\"\n";
    out << "  },\n";
    out << "  \"paths\": {\n";
    std::size_t path_index = 0;
    for (const auto& [path, apis] : apis_by_path)
    {
        out << "    " << quoted(path) << ": {\n";
        for (std::size_t i = 0; i < apis.size(); ++i)
        {
            out << "      " << quoted(api_method(*apis[i])) << ": {\n";
            write_operation(out, openapi_system, *apis[i]);
            out << "      }" << (i + 1 == apis.size() ? "\n" : ",\n");
        }
        out << "    }" << (++path_index == apis_by_path.size() ? "\n" : ",\n");
    }
    out << "  },\n";
    out << "  \"components\": {\n";
    out << "    \"schemas\": {\n";
    for (std::size_t i = 0; i < openapi_system.shapes.size(); ++i)
    {
        write_shape_schema(out, openapi_system, openapi_system.shapes[i]);
        out << (i + 1 == openapi_system.shapes.size() ? "\n" : ",\n");
    }
    out << "    }\n";
    out << "  }\n";
    out << "}\n";
    return out.str();
}

} // namespace

GenerationResult generate_openapi(
    const Spec& spec,
    const OpenApiGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    return generate_openapi(lower_to_ir(spec), options, diagnostics);
}

GenerationResult generate_openapi(
    const IrSystem& system,
    const OpenApiGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    if (system.name.empty())
    {
        diagnostics.error(SourceRange{}, "SSPEC5301", "expected system declaration");
        return result;
    }
    if (options.output_dir.empty())
    {
        diagnostics.error(SourceRange{}, "SSPEC5302", "openapi generation requires output_dir");
        return result;
    }

    result.files.push_back(
        GeneratedFile{
            (options.output_dir / "openapi.json").string(),
            render_openapi(system),
            GeneratedArtifactTier::Api,
            "api/openapi.json",
        }
    );
    return result;
}

} // namespace statespec
