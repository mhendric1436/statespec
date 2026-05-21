#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

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

std::string generate_go_api_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "func ApiDescriptors() []ApiDescriptor {\n";
    out << "\treturn []ApiDescriptor{\n";
    for (const auto& api : system.apis)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(api.name) << ",\n";
        out << "\t\t\tMethod: " << string_ptr_expr(api.method) << ",\n";
        out << "\t\t\tPath: " << string_ptr_expr(api.path) << ",\n";
        out << "\t\t\tInput: " << string_ptr_expr(api.input) << ",\n";
        out << "\t\t\tOutput: " << string_ptr_expr(api.output) << ",\n";
        out << "\t\t\tError: " << string_ptr_expr(api.error) << ",\n";
        out << "\t\t\tStartsWorkflow: " << string_ptr_expr(api.starts_workflow) << ",\n";
        out << "\t\t\tEnqueues: " << string_ptr_expr(api.enqueues) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func ApiServerDescriptors() []ApiServerDescriptor {\n";
    out << "\treturn []ApiServerDescriptor{\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(api_server.name) << ",\n";
        out << "\t\t\tServes: []string{";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(api_server.serves[i]);
        }
        out << "},\n";
        out << "\t\t\tConcurrency: " << api_server.concurrency.value_or(1) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func ApiRouteDescriptors() []ApiRouteDescriptor {\n";
    out << "\treturn []ApiRouteDescriptor{\n";
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api_name : api_server.serves)
        {
            const auto* api = find_go_api(system, api_name);
            out << "\t\t{\n";
            out << "\t\t\tName: " << go_string(api_server.name + "." + api_name) << ",\n";
            out << "\t\t\tServerName: " << go_string(api_server.name) << ",\n";
            out << "\t\t\tApiName: " << go_string(api_name) << ",\n";
            out << "\t\t\tMethod: " << string_ptr_expr(optional_api_field(api, &IrApi::method))
                << ",\n";
            out << "\t\t\tPath: " << string_ptr_expr(optional_api_field(api, &IrApi::path))
                << ",\n";
            out << "\t\t\tInput: " << string_ptr_expr(optional_api_field(api, &IrApi::input))
                << ",\n";
            out << "\t\t\tOutput: " << string_ptr_expr(optional_api_field(api, &IrApi::output))
                << ",\n";
            out << "\t\t\tError: " << string_ptr_expr(optional_api_field(api, &IrApi::error))
                << ",\n";
            out << "\t\t},\n";
        }
    }
    out << "\t}\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
