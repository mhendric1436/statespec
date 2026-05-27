#include "semantic_entities.hpp"

#include "semantic_symbols.hpp"

#include <string_view>
#include <utility>

namespace statespec
{

namespace
{

std::string relation_target_name(std::string target)
{
    if (!target.empty() && target.back() == '?')
    {
        target.pop_back();
    }
    constexpr std::string_view optional_prefix{"optional "};
    if (target.rfind(std::string{optional_prefix}, 0) == 0)
    {
        target = target.substr(optional_prefix.size());
    }
    constexpr std::string_view ref_prefix{"ref<"};
    if (target.rfind(std::string{ref_prefix}, 0) == 0 && target.back() == '>')
    {
        return target.substr(ref_prefix.size(), target.size() - ref_prefix.size() - 1);
    }
    return target;
}

SemanticEntity resolve_entity(
    const SymbolTable& symbols,
    const EntityDecl& entity
)
{
    SemanticEntity resolved;
    resolved.name = entity.name;
    resolved.key_fields = entity.key_fields;
    resolved.fields = resolve_fields(entity.fields);
    for (const auto& index : entity.indexes)
    {
        resolved.indexes.push_back(SemanticIndex{index.name, index.fields, index.unique});
    }
    if (entity.ownership.has_value())
    {
        resolved.ownership = SemanticOwnership{
            entity.ownership->authority.value_or(""),
            entity.ownership->system_of_record.value_or(""),
            entity.ownership->lifecycle.value_or(""),
        };
    }
    for (const auto& relation : entity.relations)
    {
        resolved.relations.push_back(
            SemanticRelation{
                relation.kind,
                relation.name,
                resolve_reference(symbols, relation_target_name(relation.target)),
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
        resolved.children.push_back(
            SemanticChild{
                child.name,
                resolve_reference(symbols, child.target_entity),
                child.relation,
            }
        );
    }
    for (const auto& invariant : entity.invariants)
    {
        resolved.invariants.push_back(SemanticInvariant{invariant.name, invariant.expression});
    }

    if (entity.state_machine.has_value())
    {
        const auto& state_machine = *entity.state_machine;
        resolved.initial_state = state_machine.initial_state;
        resolved.terminal_states = state_machine.terminal_states;
        for (const auto& state : state_machine.states)
        {
            SemanticState resolved_state;
            resolved_state.name = state.name;
            resolved_state.terminal = state.terminal;
            if (state.garbage_collection.has_value())
            {
                resolved_state.garbage_collection = SemanticGarbageCollectionPolicy{
                    state.garbage_collection->after.value_or(""),
                    state.garbage_collection->mode.value_or(""),
                };
            }
            resolved.states.push_back(std::move(resolved_state));
        }
        for (const auto& transition : state_machine.transitions)
        {
            resolved.transitions.push_back(SemanticTransition{transition.from, transition.to});
        }
    }

    if (entity.api.has_value())
    {
        SemanticEntityApi api;
        api.resource = entity.api->resource;
        if (entity.api->create.has_value())
        {
            api.create = SemanticEntityApiCreate{
                entity.api->create->name,
                entity.api->create->fields,
                entity.api->create->response_fields,
            };
        }
        if (entity.api->get.has_value())
        {
            api.get = SemanticEntityApiOperation{
                entity.api->get->name,
                entity.api->get->response_fields,
            };
        }
        for (const auto& list : entity.api->lists)
        {
            api.lists.push_back(
                SemanticEntityApiList{
                    list.name,
                    list.path,
                    list.by,
                    list.response_fields,
                }
            );
        }
        if (entity.api->update_status.has_value())
        {
            api.update_status = SemanticEntityApiOperation{
                entity.api->update_status->name,
                entity.api->update_status->response_fields,
            };
        }
        if (entity.api->delete_.has_value())
        {
            api.delete_ = SemanticEntityApiOperation{
                entity.api->delete_->name,
                entity.api->delete_->response_fields,
            };
        }
        resolved.api = std::move(api);
    }

    return resolved;
}

} // namespace

void resolve_semantic_entities(
    const SystemDecl& system,
    const SymbolTable& symbols,
    SemanticSystem& resolved
)
{
    for (const auto& entity : system.entities)
    {
        resolved.entities.push_back(resolve_entity(symbols, entity));
    }
}

} // namespace statespec
