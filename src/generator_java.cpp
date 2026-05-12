#include "statespec/generator_java.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

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

GenerationResult generate_java_bindings(
    const Spec&,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/java/com/statespec/backend"};
    const std::filesystem::path output_root{"com/statespec/backend"};

    add_template_file(result, options.output_dir, template_root / "Backend.java", output_root / "Backend.java", diagnostics);
    add_template_file(result, options.output_dir, template_root / "Lease.java", output_root / "Lease.java", diagnostics);
    add_template_file(result, options.output_dir, template_root / "Queue.java", output_root / "Queue.java", diagnostics);
    add_template_file(result, options.output_dir, template_root / "Workflow.java", output_root / "Workflow.java", diagnostics);

    return result;
}

} // namespace statespec
