#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_entity_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "func EntityDescriptors() []EntityDescriptor {\n";
    out << "\treturn []EntityDescriptor{\n";
    for (const auto& entity : system.entities)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(entity.name) << ",\n";
        out << "\t\t\tKeyFields: []string{";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(entity.key_fields[i]);
        }
        out << "},\n";
        if (entity.ownership.has_value())
        {
            out << "\t\t\tOwnership: &EntityOwnershipDescriptor{Authority: "
                << go_string(entity.ownership->authority)
                << ", SystemOfRecord: " << go_string(entity.ownership->system_of_record)
                << ", Lifecycle: " << go_string(entity.ownership->lifecycle) << "},\n";
        }
        else
        {
            out << "\t\t\tOwnership: nil,\n";
        }
        out << "\t\t\tRelations: []EntityRelationDescriptor{\n";
        for (const auto& relation : entity.relations)
        {
            out << "\t\t\t\t{\n";
            out << "\t\t\t\t\tKind: " << go_string(relation.kind) << ",\n";
            out << "\t\t\t\t\tName: " << go_string(relation.name) << ",\n";
            out << "\t\t\t\t\tTarget: " << go_string(relation.target) << ",\n";
            out << "\t\t\t\t\tOptional: " << (relation.optional ? "true" : "false") << ",\n";
            out << "\t\t\t\t\tRelationKind: " << string_ptr_expr(relation.relation_kind) << ",\n";
            out << "\t\t\t\t\tOnParentDelete: " << string_ptr_expr(relation.on_parent_delete)
                << ",\n";
            out << "\t\t\t\t\tParentMustBeIn: []string{";
            for (std::size_t i = 0; i < relation.parent_must_be_in.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_string(relation.parent_must_be_in[i]);
            }
            out << "},\n";
            out << "\t\t\t\t\tUniqueWithinParent: []string{";
            for (std::size_t i = 0; i < relation.unique_within_parent.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_string(relation.unique_within_parent[i]);
            }
            out << "},\n";
            out << "\t\t\t\t},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tChildren: []EntityChildDescriptor{\n";
        for (const auto& child : entity.children)
        {
            out << "\t\t\t\t{Name: " << go_string(child.name)
                << ", TargetEntity: " << go_string(child.target_entity)
                << ", Relation: " << go_string(child.relation) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tInvariants: []EntityInvariantDescriptor{\n";
        for (const auto& invariant : entity.invariants)
        {
            out << "\t\t\t\t{Name: " << go_string(invariant.name)
                << ", Expression: " << go_string(invariant.expression) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tStates: []EntityStateDescriptor{\n";
        for (const auto& state : entity.states)
        {
            out << "\t\t\t\t{\n";
            out << "\t\t\t\t\tName: " << go_string(state.name) << ",\n";
            out << "\t\t\t\t\tTerminal: " << (state.terminal ? "true" : "false") << ",\n";
            if (state.garbage_collection.has_value())
            {
                out << "\t\t\t\t\tGarbageCollection: &GarbageCollectionPolicy{After: "
                    << go_string(state.garbage_collection->after)
                    << ", Mode: " << go_string(state.garbage_collection->mode) << "},\n";
            }
            else
            {
                out << "\t\t\t\t\tGarbageCollection: nil,\n";
            }
            out << "\t\t\t\t},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tInitialState: " << string_ptr_expr(entity.initial_state) << ",\n";
        out << "\t\t\tTerminalStates: []string{";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(entity.terminal_states[i]);
        }
        out << "},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func CollectionDescriptors() []CollectionDescriptor {\n";
    out << "\treturn []CollectionDescriptor{\n";

    for (const auto& entity : system.entities)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(entity.name) << ",\n";
        out << "\t\t\tFields: []FieldDescriptor{\n";
        for (const auto& field : entity.fields)
        {
            out << "\t\t\t\t" << go_field_descriptor_expr(field) << ",\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tKeyFields: []string{";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(entity.key_fields[i]);
        }
        out << "},\n";
        out << "\t\t\tIndexes: []IndexDescriptor{\n";
        for (const auto& index : entity.indexes)
        {
            out << "\t\t\t\t{\n";
            out << "\t\t\t\t\tName: " << go_string(index.name) << ",\n";
            out << "\t\t\t\t\tFields: []string{";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_string(index.fields[i]);
            }
            out << "},\n";
            out << "\t\t\t\t\tUnique: " << (index.unique ? "true" : "false") << ",\n";
            out << "\t\t\t\t},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tSchemaVersion: 1,\n";
        out << "\t\t},\n";
    }

    out << "\t}\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
