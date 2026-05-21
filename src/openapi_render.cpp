#include "openapi_render.hpp"

#include "openapi_json.hpp"
#include "openapi_operator_metadata.hpp"
#include "openapi_paths.hpp"
#include "openapi_schema.hpp"

#include <map>
#include <sstream>
#include <vector>

namespace statespec
{

std::string render_openapi(const IrSystem& system)
{
    const auto openapi_system = with_generated_operator_metadata_openapi(system);
    std::map<std::string, std::vector<const IrApi*>> apis_by_path;
    for (const auto& api : openapi_system.apis)
    {
        apis_by_path[openapi_api_path(api)].push_back(&api);
    }

    std::ostringstream out;
    out << "{\n";
    out << "  \"openapi\": \"3.0.3\",\n";
    out << "  \"info\": {\n";
    out << "    \"title\": " << openapi_quoted(openapi_system.name + " API") << ",\n";
    out << "    \"version\": \"0.1.0\"\n";
    out << "  },\n";
    out << "  \"paths\": {\n";
    std::size_t path_index = 0;
    for (const auto& [path, apis] : apis_by_path)
    {
        out << "    " << openapi_quoted(path) << ": {\n";
        for (std::size_t i = 0; i < apis.size(); ++i)
        {
            out << "      " << openapi_quoted(openapi_api_method(*apis[i])) << ": {\n";
            write_openapi_operation(out, openapi_system, *apis[i]);
            out << "      }" << (i + 1 == apis.size() ? "\n" : ",\n");
        }
        out << "    }" << (++path_index == apis_by_path.size() ? "\n" : ",\n");
    }
    out << "  },\n";
    out << "  \"components\": {\n";
    out << "    \"schemas\": {\n";
    for (std::size_t i = 0; i < openapi_system.shapes.size(); ++i)
    {
        write_openapi_shape_schema(out, openapi_system, openapi_system.shapes[i]);
        out << (i + 1 == openapi_system.shapes.size() ? "\n" : ",\n");
    }
    out << "    }\n";
    out << "  }\n";
    out << "}\n";
    return out.str();
}

} // namespace statespec
