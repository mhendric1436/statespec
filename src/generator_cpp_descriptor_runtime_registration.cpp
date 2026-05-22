#include "generator_cpp_descriptor_areas.hpp"

namespace statespec
{

std::string generate_cpp_runtime_registration(
    const IrSystem&,
    const TemplatePackage& templates
)
{
    return templates.load("generated/runtime_registration.hpp.tmpl");
}

} // namespace statespec
