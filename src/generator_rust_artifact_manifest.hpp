#pragma once

#include "statespec/generator_bindings.hpp"
#include "statespec/template_renderer.hpp"

namespace statespec
{

TemplateRenderer::Values rust_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
);

TemplateRenderer::Values rust_lib_values(
    BindingGenerationTier tier,
    const IrSystem& system
);

} // namespace statespec
