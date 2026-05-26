#include "generator_java_descriptor_areas.hpp"

#include "generator_entity_index_helpers.hpp"
#include "generator_java_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string java_entity_model_ref(const IrEntity& entity)
{
    return "com.statespec.generated.entities." + snake_identifier(entity.name) + ".Model";
}

std::string java_entity_model_constant(
    const IrEntity& entity,
    const std::string& constant_name
)
{
    return java_entity_model_ref(entity) + "." + constant_name;
}

} // namespace

std::string generate_java_entity_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<EntityDescriptor> entityDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t entity_index = 0; entity_index < system.entities.size(); ++entity_index)
    {
        const auto& entity = system.entities[entity_index];
        out << "            new EntityDescriptor(\n";
        out << "                "
            << java_entity_model_constant(entity, java_entity_name_constant_name(entity.name))
            << ",\n";
        out << "                List.of(";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_entity_model_constant(
                entity, java_entity_field_constant_name(entity.name, entity.key_fields[i])
            );
        }
        out << "),\n";
        if (entity.ownership.has_value())
        {
            out << "                Optional.of(new EntityOwnershipDescriptor(\n";
            out << "                    " << java_string(entity.ownership->authority) << ",\n";
            out << "                    " << java_string(entity.ownership->system_of_record)
                << ",\n";
            out << "                    " << java_string(entity.ownership->lifecycle) << "\n";
            out << "                )),\n";
        }
        else
        {
            out << "                Optional.empty(),\n";
        }
        out << "                List.of(\n";
        for (std::size_t i = 0; i < entity.relations.size(); ++i)
        {
            const auto& relation = entity.relations[i];
            out << "                    new EntityRelationDescriptor(\n";
            out << "                        " << java_string(relation.kind) << ",\n";
            out << "                        " << java_string(relation.name) << ",\n";
            out << "                        " << java_string(relation.target) << ",\n";
            out << "                        " << (relation.optional ? "true" : "false") << ",\n";
            out << "                        " << java_optional_string_expr(relation.relation_kind)
                << ",\n";
            out << "                        "
                << java_optional_string_expr(relation.on_parent_delete) << ",\n";
            out << "                        List.of(";
            for (std::size_t state_index = 0; state_index < relation.parent_must_be_in.size();
                 ++state_index)
            {
                if (state_index > 0)
                {
                    out << ", ";
                }
                out << java_string(relation.parent_must_be_in[state_index]);
            }
            out << "),\n";
            out << "                        List.of(";
            for (std::size_t field_index = 0; field_index < relation.unique_within_parent.size();
                 ++field_index)
            {
                if (field_index > 0)
                {
                    out << ", ";
                }
                out << java_string(relation.unique_within_parent[field_index]);
            }
            out << ")\n";
            out << "                    )" << (i + 1 < entity.relations.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < entity.children.size(); ++i)
        {
            const auto& child = entity.children[i];
            out << "                    new EntityChildDescriptor(" << java_string(child.name)
                << ", " << java_string(child.target_entity) << ", " << java_string(child.relation)
                << ")";
            out << (i + 1 < entity.children.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < entity.invariants.size(); ++i)
        {
            const auto& invariant = entity.invariants[i];
            out << "                    new EntityInvariantDescriptor("
                << java_string(invariant.name) << ", " << java_string(invariant.expression) << ")";
            out << (i + 1 < entity.invariants.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < entity.states.size(); ++i)
        {
            const auto& state = entity.states[i];
            out << "                    new EntityStateDescriptor(\n";
            out << "                        "
                << java_entity_model_constant(
                       entity, java_entity_state_constant_name(entity.name, state.name)
                   )
                << ",\n";
            out << "                        " << (state.terminal ? "true" : "false") << ",\n";
            if (state.garbage_collection.has_value())
            {
                out << "                        Optional.of(new GarbageCollectionPolicy("
                    << java_string(state.garbage_collection->after) << ", "
                    << java_string(state.garbage_collection->mode) << "))\n";
            }
            else
            {
                out << "                        Optional.empty()\n";
            }
            out << "                    )" << (i + 1 < entity.states.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        if (entity.initial_state.has_value())
        {
            out << "                Optional.of("
                << java_entity_model_constant(
                       entity, java_entity_state_constant_name(entity.name, *entity.initial_state)
                   )
                << "),\n";
        }
        else
        {
            out << "                Optional.empty(),\n";
        }
        out << "                List.of(";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_entity_model_constant(
                entity, java_entity_state_constant_name(entity.name, entity.terminal_states[i])
            );
        }
        out << ")\n";
        out << "            )" << (entity_index + 1 < system.entities.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    for (const auto& entity : system.entities)
    {
        out << "    public static CollectionDescriptor " << lower_camel_identifier(entity.name)
            << "CollectionDescriptor() {\n";
        out << "        return com.statespec.generated.entities." << snake_identifier(entity.name)
            << ".Schema." << lower_camel_identifier(entity.name) << "CollectionDescriptor();\n";
        out << "    }\n\n";
        out << "    public static EntityDescriptor " << lower_camel_identifier(entity.name)
            << "EntityDescriptor() {\n";
        out << "        return entityDescriptors().stream()\n";
        out << "            .filter(descriptor -> descriptor.name().equals("
            << java_entity_model_constant(entity, java_entity_name_constant_name(entity.name))
            << "))\n";
        out << "            .findFirst()\n";
        out << "            .orElseThrow(() -> new IllegalStateException(\"entity descriptor not "
               "found: "
            << "\" + "
            << java_entity_model_constant(entity, java_entity_name_constant_name(entity.name))
            << "));\n";
        out << "    }\n\n";
    }

    return out.str();
}

} // namespace statespec
