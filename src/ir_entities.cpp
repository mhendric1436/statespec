#include "ir_entities.hpp"

#include "ir_lowering_helpers.hpp"

#include <utility>

namespace statespec
{

void lower_ir_entities(
    const SemanticSystem& system,
    IrSystem& ir
)
{
    for (const auto& entity : system.entities)
    {
        IrEntity ir_entity;
        ir_entity.name = entity.name;
        ir_entity.key_fields = entity.key_fields;
        ir_entity.fields = lower_fields(entity.fields);
        for (const auto& index : entity.indexes)
        {
            ir_entity.indexes.push_back(IrIndex{index.name, index.fields, index.unique});
        }
        if (entity.ownership.has_value())
        {
            ir_entity.ownership = IrOwnership{
                entity.ownership->authority,
                entity.ownership->system_of_record,
                entity.ownership->lifecycle,
            };
        }
        for (const auto& relation : entity.relations)
        {
            ir_entity.relations.push_back(
                IrRelation{
                    relation.kind,
                    relation.name,
                    relation.target.name,
                    relation.optional,
                    relation.relation_kind,
                    relation.on_parent_delete,
                    relation.parent_must_be_in,
                    relation.unique_within_parent,
                }
            );
        }
        for (const auto& child : entity.children)
        {
            ir_entity.children.push_back(
                IrChild{child.name, child.target_entity.name, child.relation}
            );
        }
        for (const auto& invariant : entity.invariants)
        {
            ir_entity.invariants.push_back(IrInvariant{invariant.name, invariant.expression});
        }
        ir_entity.initial_state = entity.initial_state;
        ir_entity.terminal_states = entity.terminal_states;
        for (const auto& state : entity.states)
        {
            IrState ir_state;
            ir_state.name = state.name;
            ir_state.terminal = state.terminal;
            if (state.garbage_collection.has_value())
            {
                ir_state.garbage_collection = IrGarbageCollectionPolicy{
                    state.garbage_collection->after,
                    state.garbage_collection->mode,
                };
            }
            ir_entity.states.push_back(std::move(ir_state));
        }
        for (const auto& transition : entity.transitions)
        {
            ir_entity.transitions.push_back(IrTransition{transition.from, transition.to});
        }
        ir.entities.push_back(std::move(ir_entity));
    }
}

} // namespace statespec
