#include "generator_go_descriptor_areas.hpp"

namespace statespec
{

std::string generate_go_runtime_registration(
    const IrSystem&,
    const TemplatePackage& templates
)
{
    return templates.load("generated/runtime_registration.go.tmpl");
}

} // namespace statespec
