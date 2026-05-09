#include "generator_backend.hpp"

#include <set>
#include <vector>

namespace statespec::generator_backend
{

namespace
{

std::string yaml_quote(const std::string& value)
{
    std::ostringstream out;
    out << '"';
    for (const auto ch : value)
    {
        if (ch == '"' || ch == '\\')
        {
            out << '\\';
        }
        out << ch;
    }
    out << '"';
    return out.str();
}

std::string schema_ref(const std::string& name)
{
    return "#/components/schemas/" + name;
}

std::string openapi_type_for_statespec_type(const std::string& type)
{
    const auto base = strip_optional_suffix(type);
    if (base == "bool")
    {
        return "boolean";
    }
    if (base == "int" || base == "int32" || base == "int64" || base == "long")
    {
        return "integer";
    }
    if (base == "double" || base == "decimal")
    {
        return "number";
    }
    if (base == "json")
    {
        return "object";
    }
    return "string";
}

std::optional<std::string> openapi_format_for_statespec_type(const std::string& type)
{
    const auto base = strip_optional_suffix(type);
    if (base == "uuid")
    {
        return "uuid";
    }
    if (base == "int32" || base == "int")
    {
        return "int32";
    }
    if (base == "int64" || base == "long")
    {
        return "int64";
    }
    if (base == "double" || base == "decimal")
    {
        return "double";
    }
    if (base == "timestamp")
    {
        return "date-time";
    }
    if (base == "duration")
    {
        return "duration";
    }
    return std::nullopt;
}

std::string path_param_to_field_name(const std::string& name)
{
    std::string result;
    for (const auto ch : name)
    {
        if (std::isupper(static_cast<unsigned char>(ch)))
        {
            result += '_';
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        else
        {
            result += ch;
        }
    }
    return result;
}

std::vector<std::string> extract_path_params(const std::string& path)
{
    std::vector<std::string> params;
    std::size_t cursor = 0;
    while (cursor < path.size())
    {
        const auto open = path.find('{', cursor);
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
            params.push_back(path.substr(open + 1, close - open - 1));
        }
        cursor = close + 1;
    }
    return params;
}

std::optional<FieldDecl> find_field(
    const SystemDecl& system,
    const std::string& name
)
{
    for (const auto& entity : system.entities)
    {
        for (const auto& field : entity.fields)
        {
            if (field.name == name)
            {
                return field;
            }
        }
    }
    return std::nullopt;
}

void write_schema_property(
    std::ostream& out,
    const std::string& name,
    const std::string& type,
    const std::string& indent
)
{
    out << indent << name << ":\n";
    out << indent << "  type: " << openapi_type_for_statespec_type(type) << "\n";
    if (const auto format = openapi_format_for_statespec_type(type); format.has_value())
    {
        out << indent << "  format: " << *format << "\n";
    }
}

void write_object_schema(
    std::ostream& out,
    const std::string& name,
    const std::vector<FieldDecl>& fields,
    const std::string& description
)
{
    out << "    " << name << ":\n";
    out << "      type: object\n";
    out << "      description: " << yaml_quote(description) << "\n";

    bool has_required = false;
    for (const auto& field : fields)
    {
        if (!is_optional_type(field.type))
        {
            has_required = true;
            break;
        }
    }

    if (has_required)
    {
        out << "      required:\n";
        for (const auto& field : fields)
        {
            if (!is_optional_type(field.type))
            {
                out << "        - " << field.name << "\n";
            }
        }
    }

    out << "      properties:\n";
    if (fields.empty())
    {
        out << "        metadata:\n";
        out << "          type: object\n";
        out << "          additionalProperties: true\n";
    }
    else
    {
        for (const auto& field : fields)
        {
            write_schema_property(out, field.name, field.type, "        ");
        }
    }
}

void write_problem_details_schema(
    std::ostream& out,
    const std::string& name
)
{
    out << "    " << name << ":\n";
    out << "      type: object\n";
    out << "      description: " << yaml_quote("RFC 9457 problem details error response") << "\n";
    out << "      required:\n";
    out << "        - title\n";
    out << "        - status\n";
    out << "      properties:\n";
    out << "        type:\n";
    out << "          type: string\n";
    out << "          format: uri\n";
    out << "        title:\n";
    out << "          type: string\n";
    out << "        status:\n";
    out << "          type: integer\n";
    out << "          format: int32\n";
    out << "        detail:\n";
    out << "          type: string\n";
    out << "        instance:\n";
    out << "          type: string\n";
    out << "          format: uri\n";
}

std::vector<FieldDecl> request_fields_for_api(
    const SystemDecl& system,
    const ApiDecl& api
)
{
    std::vector<FieldDecl> fields;
    std::set<std::string> seen;

    for (const auto& param : extract_path_params(api.path.value_or("/")))
    {
        const auto field_name = path_param_to_field_name(param);
        auto field = find_field(system, field_name);
        if (!field.has_value())
        {
            field = FieldDecl{param, "string", api.range};
        }
        else
        {
            field->name = param;
        }
        if (seen.insert(field->name).second)
        {
            fields.push_back(*field);
        }
    }

    if (api.starts_workflow.has_value() && seen.insert("workflowName").second)
    {
        fields.push_back(FieldDecl{"workflowName", "string", api.range});
    }
    if (api.enqueues.has_value() && seen.insert("messageName").second)
    {
        fields.push_back(FieldDecl{"messageName", "string", api.range});
    }

    return fields;
}

std::vector<FieldDecl> response_fields_for_api(const ApiDecl& api)
{
    std::vector<FieldDecl> fields;
    fields.push_back(FieldDecl{"accepted", "bool", api.range});
    fields.push_back(FieldDecl{"operationId", "string", api.range});
    if (api.starts_workflow.has_value())
    {
        fields.push_back(FieldDecl{"workflow", "string", api.range});
    }
    if (api.enqueues.has_value())
    {
        fields.push_back(FieldDecl{"message", "string", api.range});
    }
    return fields;
}

void write_path_parameters(
    std::ostream& out,
    const SystemDecl& system,
    const ApiDecl& api
)
{
    const auto params = extract_path_params(api.path.value_or("/"));
    if (params.empty())
    {
        return;
    }

    out << "      parameters:\n";
    for (const auto& param : params)
    {
        const auto field_name = path_param_to_field_name(param);
        const auto field = find_field(system, field_name);
        const auto type = field.has_value() ? field->type : std::string{"string"};
        out << "        - name: " << param << "\n";
        out << "          in: path\n";
        out << "          required: true\n";
        out << "          schema:\n";
        out << "            type: " << openapi_type_for_statespec_type(type) << "\n";
        if (const auto format = openapi_format_for_statespec_type(type); format.has_value())
        {
            out << "            format: " << *format << "\n";
        }
    }
}

void write_api_operation(
    std::ostream& out,
    const SystemDecl& system,
    const ApiDecl& api
)
{
    const auto method = to_lower(api.method.value_or("GET"));
    const auto input_schema = api.input.value_or(api.name + "Request");
    const auto output_schema = api.output.value_or(api.name + "Response");
    const auto error_schema = api.error.value_or("ProblemDetails");
    const auto success_code =
        (api.starts_workflow.has_value() || api.enqueues.has_value()) ? "202" : "200";

    out << "    " << method << ":\n";
    out << "      operationId: " << api.name << "\n";
    out << "      summary: " << yaml_quote(api.name) << "\n";
    out << "      tags:\n";
    out << "        - " << system.name << "\n";
    write_path_parameters(out, system, api);

    if (api.input.has_value())
    {
        out << "      requestBody:\n";
        out << "        required: true\n";
        out << "        content:\n";
        out << "          application/json:\n";
        out << "            schema:\n";
        out << "              $ref: " << yaml_quote(schema_ref(input_schema)) << "\n";
    }

    out << "      responses:\n";
    out << "        \"" << success_code << "\":\n";
    out << "          description: "
        << yaml_quote(success_code == std::string{"202"} ? "Accepted" : "OK") << "\n";
    out << "          content:\n";
    out << "            application/json:\n";
    out << "              schema:\n";
    out << "                $ref: " << yaml_quote(schema_ref(output_schema)) << "\n";
    out << "        default:\n";
    out << "          description: " << yaml_quote("Problem details error response") << "\n";
    out << "          content:\n";
    out << "            application/problem+json:\n";
    out << "              schema:\n";
    out << "                $ref: " << yaml_quote(schema_ref(error_schema)) << "\n";
}

void write_components(std::ostream& out, const SystemDecl& system)
{
    out << "components:\n";
    out << "  schemas:\n";

    std::set<std::string> emitted;
    emitted.insert("ProblemDetails");
    write_problem_details_schema(out, "ProblemDetails");

    for (const auto& entity : system.entities)
    {
        if (emitted.insert(entity.name).second)
        {
            write_object_schema(out, entity.name, entity.fields, entity.name + " entity");
        }
    }

    for (const auto& queue : system.queues)
    {
        for (const auto& message : queue.messages)
        {
            const auto schema_name = payload_struct_name(queue, message);
            if (emitted.insert(schema_name).second)
            {
                write_object_schema(
                    out, schema_name, message.payload_fields,
                    queue.name + "." + message.name + " message payload"
                );
            }
        }
    }

    for (const auto& api : system.apis)
    {
        if (api.input.has_value() && emitted.insert(*api.input).second)
        {
            write_object_schema(
                out, *api.input, request_fields_for_api(system, api),
                api.name + " request model"
            );
        }
        if (api.output.has_value() && emitted.insert(*api.output).second)
        {
            write_object_schema(
                out, *api.output, response_fields_for_api(api), api.name + " response model"
            );
        }
        if (api.error.has_value() && emitted.insert(*api.error).second)
        {
            write_problem_details_schema(out, *api.error);
        }
    }
}

} // namespace

void generate_openapi(
    const SystemDecl& system,
    const GenerateDecl& declaration,
    const GenerationOptions& options,
    GenerationResult& result
)
{
    std::ostringstream out;
    out << "openapi: 3.1.0\n";
    out << "info:\n";
    out << "  title: " << yaml_quote(system.name + " API") << "\n";
    out << "  version: 0.1.0\n";
    out << "  description: " << yaml_quote("Generated by StateSpec") << "\n";
    out << "paths:\n";
    if (system.apis.empty())
    {
        out << "  {}\n";
    }
    else
    {
        for (const auto& api : system.apis)
        {
            out << "  " << yaml_quote(api.path.value_or("/")) << ":\n";
            write_api_operation(out, system, api);
        }
    }
    write_components(out, system);

    const auto root = output_root(declaration, options);
    result.files.push_back(GeneratedFile{join_path(root, "openapi.yaml"), out.str()});
}

void generate_scaffold(
    const SystemDecl& system,
    const GenerateDecl& declaration,
    const GenerationOptions& options,
    GenerationResult& result
)
{
    std::ostringstream out;
    append_header(out, system, declaration.target);
    out << "status: scaffold-only\n";

    const auto root = output_root(declaration, options);
    result.files.push_back(
        GeneratedFile{join_path(root, declaration.target + "-manifest.yaml"), out.str()}
    );
}

} // namespace statespec::generator_backend
