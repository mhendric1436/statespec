#include "statespec/generator_cpp.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace statespec
{

namespace
{

std::string read_template_file(
    const std::filesystem::path& path,
    DiagnosticBag& diagnostics
)
{
    std::ifstream input(path);
    if (!input)
    {
        diagnostics.error(SourceRange{}, "SSPEC5201", "failed to read binding template: " + path.string());
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void add_template_file(
    GenerationResult& result,
    const std::filesystem::path& output_dir,
    const std::filesystem::path& template_path,
    const std::filesystem::path& relative_output_path,
    DiagnosticBag& diagnostics
)
{
    const auto content = read_template_file(template_path, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }

    result.files.push_back(GeneratedFile{
        (output_dir / relative_output_path).string(),
        content,
    });
}

} // namespace

GenerationResult generate_cpp_bindings(
    const Spec&,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/cpp"};

    add_template_file(result, options.output_dir, template_root / "backend.hpp", "backend.hpp", diagnostics);
    add_template_file(result, options.output_dir, template_root / "lease.hpp", "lease.hpp", diagnostics);
    add_template_file(result, options.output_dir, template_root / "queue.hpp", "queue.hpp", diagnostics);
    add_template_file(result, options.output_dir, template_root / "workflow.hpp", "workflow.hpp", diagnostics);

    return result;
}

} // namespace statespec
