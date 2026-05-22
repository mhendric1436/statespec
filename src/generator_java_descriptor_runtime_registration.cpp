#include "generator_java_descriptor_areas.hpp"

namespace statespec
{

std::string generate_java_runtime_registration(
    const IrSystem&,
    const TemplatePackage& templates
)
{
    return templates.load("generated/RuntimeRegistration.java.tmpl");
}

} // namespace statespec
