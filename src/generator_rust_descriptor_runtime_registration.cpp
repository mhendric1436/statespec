#include "generator_rust_descriptor_areas.hpp"

namespace statespec
{

std::string generate_rust_runtime_registration(
    const IrSystem&,
    const TemplatePackage& templates
)
{
    return templates.load("generated/runtime_registration.rs.tmpl");
}

} // namespace statespec
