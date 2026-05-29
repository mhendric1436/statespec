#pragma once

#include "statespec/generator_bindings.hpp"
#include "statespec/template_renderer.hpp"

namespace statespec
{

TemplateRenderer::Values java_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
);

} // namespace statespec
