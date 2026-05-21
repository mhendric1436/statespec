#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

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

std::string generate_rust_api_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn api_descriptors() -> Vec<ApiDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& api : system.apis)
    {
        out << "        ApiDescriptor {\n";
        out << "            name: " << rust_string(api.name) << ".to_string(),\n";
        out << "            method: " << rust_optional_string_expr(api.method) << ",\n";
        out << "            path: " << rust_optional_string_expr(api.path) << ",\n";
        out << "            input: " << rust_optional_string_expr(api.input) << ",\n";
        out << "            output: " << rust_optional_string_expr(api.output) << ",\n";
        out << "            error: " << rust_optional_string_expr(api.error) << ",\n";
        out << "            starts_workflow: " << rust_optional_string_expr(api.starts_workflow)
            << ",\n";
        out << "            enqueues: " << rust_optional_string_expr(api.enqueues) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn api_server_descriptors() -> Vec<ApiServerDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "        ApiServerDescriptor {\n";
        out << "            name: " << rust_string(api_server.name) << ".to_string(),\n";
        out << "            serves: vec![";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(api_server.serves[i]) << ".to_string()";
        }
        out << "],\n";
        out << "            concurrency: " << api_server.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn api_route_descriptors() -> Vec<ApiRouteDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api_name : api_server.serves)
        {
            const auto* api = find_rust_api(system, api_name);
            out << "        ApiRouteDescriptor {\n";
            out << "            name: " << rust_string(api_server.name + "." + api_name)
                << ".to_string(),\n";
            out << "            server_name: " << rust_string(api_server.name) << ".to_string(),\n";
            out << "            api_name: " << rust_string(api_name) << ".to_string(),\n";
            out << "            method: "
                << rust_optional_string_expr(optional_api_field(api, &IrApi::method)) << ",\n";
            out << "            path: "
                << rust_optional_string_expr(optional_api_field(api, &IrApi::path)) << ",\n";
            out << "            input: "
                << rust_optional_string_expr(optional_api_field(api, &IrApi::input)) << ",\n";
            out << "            output: "
                << rust_optional_string_expr(optional_api_field(api, &IrApi::output)) << ",\n";
            out << "            error: "
                << rust_optional_string_expr(optional_api_field(api, &IrApi::error)) << ",\n";
            out << "        },\n";
        }
    }
    out << "    ]\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
