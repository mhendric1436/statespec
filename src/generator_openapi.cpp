#include "generator_backend.hpp"

namespace statespec::generator_backend
{

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
    out << "  title: " << system.name << " API\n";
    out << "  version: 0.1.0\n";
    out << "paths:\n";
    for (const auto& api : system.apis)
    {
        out << "  \"" << api.path.value_or("/") << "\":\n";
        out << "    " << api.method.value_or("GET") << ":\n";
        out << "      operationId: " << api.name << "\n";
        out << "      responses:\n";
        out << "        \"200\":\n";
        out << "          description: generated stub\n";
    }

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
