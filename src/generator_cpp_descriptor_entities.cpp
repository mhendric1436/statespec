#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_entity_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<EntityDescriptor> entity_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& entity : system.entities)
    {
        out << "        EntityDescriptor{\n";
        out << "            " << cpp_string(entity.name) << ",\n";
        out << "            {";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_string(entity.key_fields[i]);
        }
        out << "},\n";
        if (entity.ownership.has_value())
        {
            out << "            std::optional<EntityOwnershipDescriptor>{EntityOwnershipDescriptor{"
                << cpp_string(entity.ownership->authority) << ", "
                << cpp_string(entity.ownership->system_of_record) << ", "
                << cpp_string(entity.ownership->lifecycle) << "}},\n";
        }
        else
        {
            out << "            std::nullopt,\n";
        }
        out << "            {\n";
        for (const auto& relation : entity.relations)
        {
            out << "                EntityRelationDescriptor{\n";
            out << "                    " << cpp_string(relation.kind) << ",\n";
            out << "                    " << cpp_string(relation.name) << ",\n";
            out << "                    " << cpp_string(relation.target) << ",\n";
            out << "                    " << (relation.optional ? "true" : "false") << ",\n";
            out << "                    " << optional_string_expr(relation.relation_kind) << ",\n";
            out << "                    " << optional_string_expr(relation.on_parent_delete)
                << ",\n";
            out << "                    {";
            for (std::size_t i = 0; i < relation.parent_must_be_in.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << cpp_string(relation.parent_must_be_in[i]);
            }
            out << "},\n";
            out << "                    {";
            for (std::size_t i = 0; i < relation.unique_within_parent.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << cpp_string(relation.unique_within_parent[i]);
            }
            out << "},\n";
            out << "                },\n";
        }
        out << "            },\n";
        out << "            {\n";
        for (const auto& child : entity.children)
        {
            out << "                EntityChildDescriptor{" << cpp_string(child.name) << ", "
                << cpp_string(child.target_entity) << ", " << cpp_string(child.relation) << "},\n";
        }
        out << "            },\n";
        out << "            {\n";
        for (const auto& invariant : entity.invariants)
        {
            out << "                EntityInvariantDescriptor{" << cpp_string(invariant.name)
                << ", " << cpp_string(invariant.expression) << "},\n";
        }
        out << "            },\n";
        out << "            {\n";
        for (const auto& state : entity.states)
        {
            out << "                EntityStateDescriptor{\n";
            out << "                    " << cpp_string(state.name) << ",\n";
            out << "                    " << (state.terminal ? "true" : "false") << ",\n";
            if (state.garbage_collection.has_value())
            {
                out << "                    std::optional<GarbageCollectionPolicy>{"
                    << "GarbageCollectionPolicy{" << cpp_string(state.garbage_collection->after)
                    << ", " << cpp_string(state.garbage_collection->mode) << "}},\n";
            }
            else
            {
                out << "                    std::nullopt,\n";
            }
            out << "                },\n";
        }
        out << "            },\n";
        out << "            " << optional_string_expr(entity.initial_state) << ",\n";
        out << "            {";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_string(entity.terminal_states[i]);
        }
        out << "},\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<statespec::backend::CollectionDescriptor> "
           "collection_descriptors()\n";
    out << "{\n";
    out << "    return {\n";

    for (const auto& entity : system.entities)
    {
        out << "        statespec::backend::CollectionDescriptor{\n";
        out << "            " << cpp_string(entity.name) << ",\n";
        out << "            {\n";
        for (const auto& field : entity.fields)
        {
            out << "                " << cpp_field_descriptor_expr(field) << ",\n";
        }
        out << "            },\n";
        out << "            {";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_string(entity.key_fields[i]);
        }
        out << "},\n";
        out << "            {\n";
        for (const auto& index : entity.indexes)
        {
            out << "                statespec::backend::IndexDescriptor{\n";
            out << "                    " << cpp_string(index.name) << ",\n";
            out << "                    {";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << cpp_string(index.fields[i]);
            }
            out << "},\n";
            out << "                    " << (index.unique ? "true" : "false") << ",\n";
            out << "                },\n";
        }
        out << "            },\n";
        out << "            1,\n";
        out << "        },\n";
    }

    out << "    };\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
