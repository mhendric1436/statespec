#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_entity_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn entity_descriptors() -> Vec<EntityDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& entity : system.entities)
    {
        out << "        EntityDescriptor {\n";
        out << "            name: " << rust_string(entity.name) << ".to_string(),\n";
        out << "            key_fields: vec![";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(entity.key_fields[i]) << ".to_string()";
        }
        out << "],\n";
        if (entity.ownership.has_value())
        {
            out << "            ownership: Some(EntityOwnershipDescriptor {\n";
            out << "                authority: " << rust_string(entity.ownership->authority)
                << ".to_string(),\n";
            out << "                system_of_record: "
                << rust_string(entity.ownership->system_of_record) << ".to_string(),\n";
            out << "                lifecycle: " << rust_string(entity.ownership->lifecycle)
                << ".to_string(),\n";
            out << "            }),\n";
        }
        else
        {
            out << "            ownership: None,\n";
        }
        out << "            relations: vec![\n";
        for (const auto& relation : entity.relations)
        {
            out << "                EntityRelationDescriptor {\n";
            out << "                    kind: " << rust_string(relation.kind) << ".to_string(),\n";
            out << "                    name: " << rust_string(relation.name) << ".to_string(),\n";
            out << "                    target: " << rust_string(relation.target)
                << ".to_string(),\n";
            out << "                    optional: " << (relation.optional ? "true" : "false")
                << ",\n";
            out << "                    relation_kind: "
                << rust_optional_string_expr(relation.relation_kind) << ",\n";
            out << "                    on_parent_delete: "
                << rust_optional_string_expr(relation.on_parent_delete) << ",\n";
            out << "                    parent_must_be_in: vec![";
            for (std::size_t i = 0; i < relation.parent_must_be_in.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(relation.parent_must_be_in[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                    unique_within_parent: vec![";
            for (std::size_t i = 0; i < relation.unique_within_parent.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(relation.unique_within_parent[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                },\n";
        }
        out << "            ],\n";
        out << "            children: vec![\n";
        for (const auto& child : entity.children)
        {
            out << "                EntityChildDescriptor { name: " << rust_string(child.name)
                << ".to_string(), target_entity: " << rust_string(child.target_entity)
                << ".to_string(), relation: " << rust_string(child.relation) << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            invariants: vec![\n";
        for (const auto& invariant : entity.invariants)
        {
            out << "                EntityInvariantDescriptor { name: "
                << rust_string(invariant.name)
                << ".to_string(), expression: " << rust_string(invariant.expression)
                << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            states: vec![\n";
        for (const auto& state : entity.states)
        {
            out << "                EntityStateDescriptor {\n";
            out << "                    name: " << rust_string(state.name) << ".to_string(),\n";
            out << "                    terminal: " << (state.terminal ? "true" : "false") << ",\n";
            if (state.garbage_collection.has_value())
            {
                out << "                    garbage_collection: Some(GarbageCollectionPolicy { "
                       "after: "
                    << rust_string(state.garbage_collection->after)
                    << ".to_string(), mode: " << rust_string(state.garbage_collection->mode)
                    << ".to_string() }),\n";
            }
            else
            {
                out << "                    garbage_collection: None,\n";
            }
            out << "                },\n";
        }
        out << "            ],\n";
        out << "            initial_state: " << rust_optional_string_expr(entity.initial_state)
            << ",\n";
        out << "            terminal_states: vec![";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(entity.terminal_states[i]) << ".to_string()";
        }
        out << "],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
