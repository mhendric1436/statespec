#include "generator_java_descriptor_areas.hpp"

#include "generator_entity_index_helpers.hpp"
#include "generator_java_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_entity_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& entity : system.entities)
    {
        out << "    public static final String " << java_entity_name_constant_name(entity.name)
            << " = " << java_string(entity.name) << ";\n";
        for (const auto& field : entity.fields)
        {
            out << "    public static final String "
                << java_entity_field_constant_name(entity.name, field.name) << " = "
                << java_string(field.name) << ";\n";
        }
        for (const auto& index : entity.indexes)
        {
            out << "    public static final String "
                << java_entity_index_constant_name(entity.name, index.name) << " = "
                << java_string(index.name) << ";\n";
        }
        for (const auto& state : entity.states)
        {
            out << "    public static final String "
                << java_entity_state_constant_name(entity.name, state.name) << " = "
                << java_string(state.name) << ";\n";
        }
    }
    if (!system.entities.empty())
    {
        out << "\n";
    }

    out << "    public static List<EntityDescriptor> entityDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t entity_index = 0; entity_index < system.entities.size(); ++entity_index)
    {
        const auto& entity = system.entities[entity_index];
        out << "            new EntityDescriptor(\n";
        out << "                " << java_entity_name_constant_name(entity.name) << ",\n";
        out << "                List.of(";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_entity_field_constant_name(entity.name, entity.key_fields[i]);
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
                << java_entity_state_constant_name(entity.name, state.name) << ",\n";
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
                << java_entity_state_constant_name(entity.name, *entity.initial_state) << "),\n";
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
            out << java_entity_state_constant_name(entity.name, entity.terminal_states[i]);
        }
        out << ")\n";
        out << "            )" << (entity_index + 1 < system.entities.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    for (const auto& entity : system.entities)
    {
        const auto type_name = pascal_identifier(entity.name);
        out << "    public static EntityLookup build" << type_name
            << "Lookup(List<EntityKeyValue> keyValues) {\n";
        out << "        return new EntityLookup(\n";
        out << "            " << java_entity_name_constant_name(entity.name) << ",\n";
        out << "            List.of(";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "),\n";
        out << "            List.copyOf(keyValues)\n";
        out << "        );\n";
        out << "    }\n\n";
        out << "    public interface " << type_name << "Repository {\n";
        out << "        void registerDescriptor(Backend backend) throws "
               "Backend.BackendException;\n";
        out << "        Optional<Backend.VersionedRecord> createTx(\n";
        out << "            Backend.Transaction tx,\n";
        out << "            Json document\n";
        out << "        ) throws Backend.BackendException;\n";
        out << "        Optional<Backend.VersionedRecord> getTx(\n";
        out << "            Backend.Transaction tx,\n";
        out << "            List<EntityKeyValue> keyValues\n";
        out << "        ) throws Backend.BackendException;\n";
        out << "        List<Backend.VersionedRecord> listByIndexTx(\n";
        out << "            Backend.Transaction tx,\n";
        out << "            String indexName,\n";
        out << "            List<Backend.IndexValue> values\n";
        out << "        ) throws Backend.BackendException;\n";
        for (const auto& index : entity.indexes)
        {
            out << "        List<Backend.VersionedRecord> "
                << entity_index_repository_method_name(index.name) << "(\n";
            out << "            Backend.Transaction tx,\n";
            out << "            List<Backend.IndexValue> values\n";
            out << "        ) throws Backend.BackendException;\n";
        }
        out << "        Optional<Backend.VersionedRecord> updateTx(\n";
        out << "            Backend.Transaction tx,\n";
        out << "            List<EntityKeyValue> keyValues,\n";
        out << "            Json document,\n";
        out << "            long expectedVersion\n";
        out << "        ) throws Backend.BackendException;\n";
        out << "    }\n\n";
        out << "    public static final class Default" << type_name << "Repository implements "
            << type_name << "Repository {\n";
        out << "        private final EntityRepository entities = new "
               "DefaultEntityRepository();\n\n";
        out << "        @Override public void registerDescriptor(Backend backend)\n";
        out << "            throws Backend.BackendException\n";
        out << "        {\n";
        out << "            backend.ensureCollection(" << lower_camel_identifier(entity.name)
            << "CollectionDescriptor());\n";
        out << "        }\n\n";
        out << "        @Override public Optional<Backend.VersionedRecord> createTx(\n";
        out << "            Backend.Transaction tx,\n";
        out << "            Json document\n";
        out << "        ) throws Backend.BackendException\n";
        out << "        {\n";
        out << "            return entities.createEntityTx(\n";
        out << "                tx,\n";
        out << "                new EntityCreateRequest(\n";
        out << "                    entityLookupFromDocument("
            << lower_camel_identifier(entity.name) << "EntityDescriptor(), document),\n";
        out << "                    document\n";
        out << "                )\n";
        out << "            );\n";
        out << "        }\n\n";
        out << "        @Override public Optional<Backend.VersionedRecord> getTx(\n";
        out << "            Backend.Transaction tx,\n";
        out << "            List<EntityKeyValue> keyValues\n";
        out << "        ) throws Backend.BackendException\n";
        out << "        {\n";
        out << "            return entities.getEntityTx(\n";
        out << "                tx, new EntityGetRequest(build" << type_name
            << "Lookup(keyValues))\n";
        out << "            );\n";
        out << "        }\n\n";
        out << "        @Override public List<Backend.VersionedRecord> listByIndexTx(\n";
        out << "            Backend.Transaction tx,\n";
        out << "            String indexName,\n";
        out << "            List<Backend.IndexValue> values\n";
        out << "        ) throws Backend.BackendException\n";
        out << "        {\n";
        out << "            return entities.listEntitiesByIndexTx(\n";
        out << "                tx,\n";
        out << "                new EntityListByIndexRequest("
            << java_entity_name_constant_name(entity.name) << ", indexName, values)\n";
        out << "            );\n";
        out << "        }\n";
        out << "\n";
        for (const auto& index : entity.indexes)
        {
            out << "        @Override public List<Backend.VersionedRecord> "
                << entity_index_repository_method_name(index.name) << "(\n";
            out << "            Backend.Transaction tx,\n";
            out << "            List<Backend.IndexValue> values\n";
            out << "        ) throws Backend.BackendException\n";
            out << "        {\n";
            out << "            return listByIndexTx(tx, "
                << java_entity_index_constant_name(entity.name, index.name) << ", values);\n";
            out << "        }\n";
            out << "\n";
        }
        out << "        @Override public Optional<Backend.VersionedRecord> updateTx(\n";
        out << "            Backend.Transaction tx,\n";
        out << "            List<EntityKeyValue> keyValues,\n";
        out << "            Json document,\n";
        out << "            long expectedVersion\n";
        out << "        ) throws Backend.BackendException\n";
        out << "        {\n";
        out << "            return entities.upsertEntityTx(\n";
        out << "                tx,\n";
        out << "                new EntityUpsertRequest(\n";
        out << "                    build" << type_name << "Lookup(keyValues),\n";
        out << "                    document,\n";
        out << "                    Optional.of(expectedVersion)\n";
        out << "                )\n";
        out << "            );\n";
        out << "        }\n";
        out << "    }\n\n";
        out << "    public static CollectionDescriptor " << lower_camel_identifier(entity.name)
            << "CollectionDescriptor() {\n";
        out << "        return new CollectionDescriptor(\n";
        out << "            " << java_entity_name_constant_name(entity.name) << ",\n";
        out << "            List.of(\n";
        for (std::size_t i = 0; i < entity.fields.size(); ++i)
        {
            out << "                "
                << java_entity_field_descriptor_expr(entity.name, entity.fields[i]);
            out << (i + 1 < entity.fields.size() ? "," : "") << "\n";
        }
        out << "            ),\n";
        out << "            List.of(";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "),\n";
        out << "            List.of(\n";
        for (std::size_t i = 0; i < entity.indexes.size(); ++i)
        {
            const auto& index = entity.indexes[i];
            out << "                new IndexDescriptor(\n";
            out << "                    "
                << java_entity_index_constant_name(entity.name, index.name) << ",\n";
            out << "                    List.of(";
            for (std::size_t field_index = 0; field_index < index.fields.size(); ++field_index)
            {
                if (field_index > 0)
                {
                    out << ", ";
                }
                out << java_entity_field_constant_name(entity.name, index.fields[field_index]);
            }
            out << "),\n";
            out << "                    " << (index.unique ? "true" : "false") << "\n";
            out << "                )" << (i + 1 < entity.indexes.size() ? "," : "") << "\n";
        }
        out << "            ),\n";
        out << "            1L\n";
        out << "        );\n";
        out << "    }\n\n";
        out << "    public static EntityDescriptor " << lower_camel_identifier(entity.name)
            << "EntityDescriptor() {\n";
        out << "        return entityDescriptors().stream()\n";
        out << "            .filter(descriptor -> descriptor.name().equals("
            << java_entity_name_constant_name(entity.name) << "))\n";
        out << "            .findFirst()\n";
        out << "            .orElseThrow(() -> new IllegalStateException(\"entity descriptor not "
               "found: "
            << "\" + " << java_entity_name_constant_name(entity.name) << "));\n";
        out << "    }\n\n";
    }

    return out.str();
}

} // namespace statespec
