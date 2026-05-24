#include "generator_rust_descriptor_areas.hpp"

#include "generator_entity_index_helpers.hpp"
#include "generator_rust_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_entity_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& entity : system.entities)
    {
        for (const auto& state : entity.states)
        {
            out << "pub const " << rust_entity_state_constant_name(entity.name, state.name)
                << ": &str = " << rust_string(state.name) << ";\n";
        }
    }
    if (!system.entities.empty())
    {
        out << "\n";
    }

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

    for (const auto& entity : system.entities)
    {
        const auto type_name = pascal_identifier(entity.name);
        const auto function_prefix = snake_identifier(entity.name);
        out << "pub fn " << function_prefix
            << "_lookup(key_values: Vec<EntityKeyValue>) -> EntityLookup {\n";
        out << "    EntityLookup {\n";
        out << "        entity: " << rust_string(entity.name) << ".to_string(),\n";
        out << "        key_fields: vec![";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(entity.key_fields[i]) << ".to_string()";
        }
        out << "],\n";
        out << "        key_values,\n";
        out << "    }\n";
        out << "}\n\n";
        out << "pub trait " << type_name << "Repository<B: Backend> {\n";
        out << "    fn register_descriptor(&self, backend: &B) -> BackendResult<()>;\n";
        out << "    fn create_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        document: Json,\n";
        out << "    ) -> BackendResult<Option<VersionedRecord>>;\n";
        out << "    fn get_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        key_values: Vec<EntityKeyValue>,\n";
        out << "    ) -> BackendResult<Option<VersionedRecord>>;\n";
        out << "    fn list_by_index_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        index_name: String,\n";
        out << "        values: Vec<crate::backend::IndexValue>,\n";
        out << "    ) -> BackendResult<Vec<VersionedRecord>>;\n";
        for (const auto& index : entity.indexes)
        {
            out << "    fn " << rust_entity_index_repository_method_name(index.name) << "(\n";
            out << "        &self,\n";
            out << "        tx: &mut B::Tx,\n";
            out << "        values: Vec<crate::backend::IndexValue>,\n";
            out << "    ) -> BackendResult<Vec<VersionedRecord>>;\n";
        }
        out << "\n";
        out << "    fn update_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        key_values: Vec<EntityKeyValue>,\n";
        out << "        document: Json,\n";
        out << "        expected_version: u64,\n";
        out << "    ) -> BackendResult<Option<VersionedRecord>>;\n";
        out << "}\n\n";
        out << "#[derive(Debug, Clone, Copy, Default)]\n";
        out << "pub struct Default" << type_name << "Repository;\n\n";
        out << "impl<B: Backend> " << type_name << "Repository<B> for Default" << type_name
            << "Repository {\n";
        out << "    fn register_descriptor(&self, backend: &B) -> BackendResult<()> {\n";
        out << "        backend.ensure_collection(&" << function_prefix
            << "_collection_descriptor())\n";
        out << "    }\n\n";
        out << "    fn create_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        document: Json,\n";
        out << "    ) -> BackendResult<Option<VersionedRecord>> {\n";
        out << "        <DefaultEntityRepository as EntityRepository<B>>::create_entity_tx(\n";
        out << "            &DefaultEntityRepository,\n";
        out << "            tx,\n";
        out << "            &EntityCreateRequest {\n";
        out << "                lookup: entity_lookup_from_document(&" << function_prefix
            << "_entity_descriptor(), &document),\n";
        out << "                document,\n";
        out << "            },\n";
        out << "        )\n";
        out << "    }\n\n";
        out << "    fn get_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        key_values: Vec<EntityKeyValue>,\n";
        out << "    ) -> BackendResult<Option<VersionedRecord>> {\n";
        out << "        <DefaultEntityRepository as EntityRepository<B>>::get_entity_tx(\n";
        out << "            &DefaultEntityRepository,\n";
        out << "            tx,\n";
        out << "            &EntityGetRequest { lookup: " << function_prefix
            << "_lookup(key_values) },\n";
        out << "        )\n";
        out << "    }\n\n";
        out << "    fn list_by_index_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        index_name: String,\n";
        out << "        values: Vec<crate::backend::IndexValue>,\n";
        out << "    ) -> BackendResult<Vec<VersionedRecord>> {\n";
        out << "        <DefaultEntityRepository as "
               "EntityRepository<B>>::list_entities_by_index_tx(\n";
        out << "            &DefaultEntityRepository,\n";
        out << "            tx,\n";
        out << "            &EntityListByIndexRequest {\n";
        out << "                entity: " << rust_string(entity.name) << ".to_string(),\n";
        out << "                index_name,\n";
        out << "                values,\n";
        out << "            },\n";
        out << "        )\n";
        out << "    }\n";
        out << "\n";
        for (const auto& index : entity.indexes)
        {
            out << "    fn " << rust_entity_index_repository_method_name(index.name) << "(\n";
            out << "        &self,\n";
            out << "        tx: &mut B::Tx,\n";
            out << "        values: Vec<crate::backend::IndexValue>,\n";
            out << "    ) -> BackendResult<Vec<VersionedRecord>> {\n";
            out << "        <Default" << type_name << "Repository as " << type_name
                << "Repository<B>>::list_by_index_tx(\n";
            out << "            self,\n";
            out << "            tx,\n";
            out << "            " << rust_string(index.name) << ".to_string(),\n";
            out << "            values,\n";
            out << "        )\n";
            out << "    }\n";
            out << "\n";
        }
        out << "    fn update_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        key_values: Vec<EntityKeyValue>,\n";
        out << "        document: Json,\n";
        out << "        expected_version: u64,\n";
        out << "    ) -> BackendResult<Option<VersionedRecord>> {\n";
        out << "        <DefaultEntityRepository as EntityRepository<B>>::upsert_entity_tx(\n";
        out << "            &DefaultEntityRepository,\n";
        out << "            tx,\n";
        out << "            &EntityUpsertRequest {\n";
        out << "                lookup: " << function_prefix << "_lookup(key_values),\n";
        out << "                document,\n";
        out << "                expected_version: Some(expected_version),\n";
        out << "            },\n";
        out << "        )\n";
        out << "    }\n";
        out << "}\n\n";
        out << "pub fn " << function_prefix
            << "_collection_descriptor() -> CollectionDescriptor {\n";
        out << "    CollectionDescriptor {\n";
        out << "        name: " << rust_string(entity.name) << ".to_string(),\n";
        out << "        fields: vec![\n";
        for (const auto& field : entity.fields)
        {
            out << "            " << rust_field_descriptor_expr(field) << ",\n";
        }
        out << "        ],\n";
        out << "        key_fields: vec![";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(entity.key_fields[i]) << ".to_string()";
        }
        out << "],\n";
        out << "        indexes: vec![\n";
        for (const auto& index : entity.indexes)
        {
            out << "            IndexDescriptor {\n";
            out << "                name: " << rust_string(index.name) << ".to_string(),\n";
            out << "                fields: vec![";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(index.fields[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                unique: " << (index.unique ? "true" : "false") << ",\n";
            out << "            },\n";
        }
        out << "        ],\n";
        out << "        schema_version: 1,\n";
        out << "    }\n";
        out << "}\n\n";
        out << "pub fn " << function_prefix << "_entity_descriptor() -> EntityDescriptor {\n";
        out << "    entity_descriptors()\n";
        out << "        .into_iter()\n";
        out << "        .find(|descriptor| descriptor.name == " << rust_string(entity.name)
            << ")\n";
        out << "        .expect(\"entity descriptor not found: " << entity.name << "\")\n";
        out << "}\n\n";
    }

    return out.str();
}

} // namespace statespec
