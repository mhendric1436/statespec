#include "statespec/ir.hpp"

#include "ir_declarations.hpp"
#include "ir_entities.hpp"
#include "ir_external_systems.hpp"
#include "ir_observability.hpp"
#include "ir_policies.hpp"
#include "ir_runtime.hpp"
#include "ir_workflows.hpp"

namespace statespec
{

IrSystem lower_to_ir(const SemanticSystem& system)
{
    IrSystem ir;

    lower_ir_declarations(system, ir);
    lower_ir_external_systems(system, ir);
    lower_ir_observability(system, ir);
    lower_ir_entities(system, ir);
    lower_ir_runtime(system, ir);
    lower_ir_workflows(system, ir);
    lower_ir_policies(system, ir);

    return ir;
}

IrSystem lower_to_ir(const Spec& spec)
{
    return lower_to_ir(resolve_semantics(spec));
}

} // namespace statespec
