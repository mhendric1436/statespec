#include "openapi_paths.hpp"

#include "openapi_json.hpp"
#include "openapi_schema.hpp"
#include "string_utils.hpp"

#include <ostream>
#include <vector>

namespace statespec
{

namespace
{

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

} // namespace

std::string openapi_api_path(const IrApi& api)
{
    if (api.path.has_value() && !api.path->empty())
    {
        return *api.path;
    }
    return "/" + api.name;
}

std::string openapi_api_method(const IrApi& api)
{
    if (api.method.has_value() && !api.method->empty())
    {
        return lower_ascii(*api.method);
    }
    return "post";
}

bool openapi_has_api_operation(
    const IrSystem& system,
    const std::string& method,
    const std::string& path
)
{
    const auto normalized_method = lower_ascii(method);
    for (const auto& api : system.apis)
    {
        if (openapi_api_method(api) == normalized_method && openapi_api_path(api) == path)
        {
            return true;
        }
    }
    return false;
}

void write_openapi_operation(
    std::ostream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    out << "      \"operationId\": " << openapi_quoted(api.name);
    const auto parameters = extract_path_parameters(openapi_api_path(api));
    if (!parameters.empty())
    {
        out << ",\n";
        out << "      \"parameters\": [\n";
        for (std::size_t i = 0; i < parameters.size(); ++i)
        {
            out << "        {\n";
            out << "          \"name\": " << openapi_quoted(parameters[i]) << ",\n";
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
        write_openapi_media_schema(out, system, *api.input, "        ");
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
        write_openapi_media_schema(out, system, *api.output, "          ");
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
        write_openapi_media_schema(out, system, *api.error, "          ");
        out << "\n";
        out << "        }\n";
    }
    else
    {
        out << "\n";
    }
    out << "      }\n";
}

} // namespace statespec
