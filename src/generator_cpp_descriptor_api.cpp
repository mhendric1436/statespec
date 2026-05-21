#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

#include <optional>

namespace
{

const std::optional<std::string>& optional_api_field(
    const statespec::IrApi* api,
    const std::optional<std::string> statespec::IrApi::* field
)
{
    static const std::optional<std::string> empty;
    return api == nullptr ? empty : api->*field;
}

} // namespace

namespace statespec
{

std::string generate_cpp_api_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<ApiDescriptor> api_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& api : system.apis)
    {
        out << "        ApiDescriptor{\n";
        out << "            " << cpp_string(api.name) << ",\n";
        out << "            " << optional_string_expr(api.method) << ",\n";
        out << "            " << optional_string_expr(api.path) << ",\n";
        out << "            " << optional_string_expr(api.input) << ",\n";
        out << "            " << optional_string_expr(api.output) << ",\n";
        out << "            " << optional_string_expr(api.error) << ",\n";
        out << "            " << optional_string_expr(api.starts_workflow) << ",\n";
        out << "            " << optional_string_expr(api.enqueues) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<ApiServerDescriptor> api_server_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "        ApiServerDescriptor{\n";
        out << "            " << cpp_string(api_server.name) << ",\n";
        out << "            {";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_string(api_server.serves[i]);
        }
        out << "},\n";
        out << "            " << api_server.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<ApiRouteDescriptor> api_route_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api_name : api_server.serves)
        {
            const auto* api = find_api(system, api_name);
            out << "        ApiRouteDescriptor{\n";
            out << "            " << cpp_string(api_server.name + "." + api_name) << ",\n";
            out << "            " << cpp_string(api_server.name) << ",\n";
            out << "            " << cpp_string(api_name) << ",\n";
            out << "            " << optional_string_expr(optional_api_field(api, &IrApi::method))
                << ",\n";
            out << "            " << optional_string_expr(optional_api_field(api, &IrApi::path))
                << ",\n";
            out << "            " << optional_string_expr(optional_api_field(api, &IrApi::input))
                << ",\n";
            out << "            " << optional_string_expr(optional_api_field(api, &IrApi::output))
                << ",\n";
            out << "            " << optional_string_expr(optional_api_field(api, &IrApi::error))
                << ",\n";
            out << "        },\n";
        }
    }
    out << "    };\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
