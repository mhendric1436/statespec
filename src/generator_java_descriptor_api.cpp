#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

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

std::string generate_java_api_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<ApiDescriptor> apiDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t api_index = 0; api_index < system.apis.size(); ++api_index)
    {
        const auto& api = system.apis[api_index];
        out << "            new ApiDescriptor(\n";
        out << "                " << java_string(api.name) << ",\n";
        out << "                " << java_optional_string_expr(api.method) << ",\n";
        out << "                " << java_optional_string_expr(api.path) << ",\n";
        out << "                " << java_optional_string_expr(api.input) << ",\n";
        out << "                " << java_optional_string_expr(api.output) << ",\n";
        out << "                " << java_optional_string_expr(api.error) << ",\n";
        out << "                " << java_optional_string_expr(api.starts_workflow) << ",\n";
        out << "                " << java_optional_string_expr(api.enqueues) << "\n";
        out << "            )" << (api_index + 1 < system.apis.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<ApiServerDescriptor> apiServerDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t server_index = 0; server_index < system.api_servers.size(); ++server_index)
    {
        const auto& api_server = system.api_servers[server_index];
        out << "            new ApiServerDescriptor(\n";
        out << "                " << java_string(api_server.name) << ",\n";
        out << "                List.of(";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_string(api_server.serves[i]);
        }
        out << "),\n";
        out << "                " << api_server.concurrency.value_or(1) << "\n";
        out << "            )" << (server_index + 1 < system.api_servers.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    std::size_t api_route_count = 0;
    for (const auto& api_server : system.api_servers)
    {
        api_route_count += api_server.serves.size();
    }
    out << "    public static List<ApiRouteDescriptor> apiRouteDescriptors() {\n";
    out << "        return List.of(\n";
    std::size_t api_route_index = 0;
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api_name : api_server.serves)
        {
            const auto* api = find_java_api(system, api_name);
            out << "            new ApiRouteDescriptor(\n";
            out << "                " << java_string(api_server.name + "." + api_name) << ",\n";
            out << "                " << java_string(api_server.name) << ",\n";
            out << "                " << java_string(api_name) << ",\n";
            out << "                "
                << java_optional_string_expr(optional_api_field(api, &IrApi::method)) << ",\n";
            out << "                "
                << java_optional_string_expr(optional_api_field(api, &IrApi::path)) << ",\n";
            out << "                "
                << java_optional_string_expr(optional_api_field(api, &IrApi::input)) << ",\n";
            out << "                "
                << java_optional_string_expr(optional_api_field(api, &IrApi::output)) << ",\n";
            out << "                "
                << java_optional_string_expr(optional_api_field(api, &IrApi::error)) << "\n";
            out << "            )" << (api_route_index + 1 < api_route_count ? "," : "") << "\n";
            ++api_route_index;
        }
    }
    out << "        );\n";
    out << "    }\n\n";

    return out.str();
}

} // namespace statespec
