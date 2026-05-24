#include "generator_go_descriptor_areas.hpp"

#include "generator_entity_index_helpers.hpp"
#include "generator_go_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_entity_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    if (!system.entities.empty())
    {
        out << "const (\n";
        for (const auto& entity : system.entities)
        {
            out << "\t" << go_entity_name_constant_name(entity.name) << " = "
                << go_string(entity.name) << "\n";
            for (const auto& field : entity.fields)
            {
                out << "\t" << go_entity_field_constant_name(entity.name, field.name) << " = "
                    << go_string(field.name) << "\n";
            }
            for (const auto& index : entity.indexes)
            {
                out << "\t" << go_entity_index_constant_name(entity.name, index.name) << " = "
                    << go_string(index.name) << "\n";
            }
            for (const auto& state : entity.states)
            {
                out << "\t" << go_entity_state_constant_name(entity.name, state.name) << " = "
                    << go_string(state.name) << "\n";
            }
        }
        out << ")\n\n";
    }

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

    for (const auto& entity : system.entities)
    {
        const auto type_name = pascal_identifier(entity.name);
        out << "func " << type_name << "Lookup(keyValues []EntityKeyValue) EntityLookup {\n";
        out << "\treturn EntityLookup{Entity: " << go_string(entity.name)
            << ", KeyFields: []string{";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(entity.key_fields[i]);
        }
        out << "}, KeyValues: keyValues}\n";
        out << "}\n\n";
        out << "type " << type_name << "Repository interface {\n";
        out << "\tRegisterDescriptor(context.Context, Backend) error\n";
        out << "\tCreateTx(context.Context, Transaction, JSON) (*VersionedRecord, error)\n";
        out << "\tGetTx(context.Context, Transaction, []EntityKeyValue) (*VersionedRecord, "
               "error)\n";
        out << "\tListByIndexTx(context.Context, Transaction, string, []IndexValue) "
               "([]VersionedRecord, error)\n";
        for (const auto& index : entity.indexes)
        {
            out << "\t" << go_entity_index_repository_method_name(index.name)
                << "(context.Context, Transaction, []IndexValue) ([]VersionedRecord, error)\n";
        }
        out << "\tUpdateTx(context.Context, Transaction, []EntityKeyValue, JSON, Version) "
               "(*VersionedRecord, error)\n";
        out << "}\n\n";
        out << "type Default" << type_name << "Repository struct{}\n\n";
        out << "func (Default" << type_name
            << "Repository) RegisterDescriptor(ctx context.Context, backend Backend) error {\n";
        out << "\treturn backend.EnsureCollection(ctx, " << type_name
            << "CollectionDescriptor())\n";
        out << "}\n\n";
        out << "func (Default" << type_name
            << "Repository) CreateTx(ctx context.Context, tx Transaction, document JSON) "
               "(*VersionedRecord, error) {\n";
        out << "\treturn DefaultEntityRepository{}.CreateEntityTx(ctx, tx, EntityCreateRequest{\n";
        out << "\t\tLookup: EntityLookupFromDocument(" << type_name
            << "EntityDescriptor(), document),\n";
        out << "\t\tDocument: document,\n";
        out << "\t})\n";
        out << "}\n\n";
        out << "func (Default" << type_name
            << "Repository) GetTx(ctx context.Context, tx Transaction, keyValues []EntityKeyValue) "
               "(*VersionedRecord, error) {\n";
        out << "\treturn DefaultEntityRepository{}.GetEntityTx(ctx, tx, EntityGetRequest{Lookup: "
            << type_name << "Lookup(keyValues)})\n";
        out << "}\n\n";
        out << "func (Default" << type_name
            << "Repository) ListByIndexTx(ctx context.Context, tx Transaction, indexName string, "
               "values []IndexValue) ([]VersionedRecord, error) {\n";
        out << "\treturn DefaultEntityRepository{}.ListEntitiesByIndexTx(ctx, tx, "
               "EntityListByIndexRequest{Entity: "
            << go_string(entity.name) << ", IndexName: indexName, Values: values})\n";
        out << "}\n\n";
        for (const auto& index : entity.indexes)
        {
            out << "func (repository Default" << type_name << "Repository) "
                << go_entity_index_repository_method_name(index.name)
                << "(ctx context.Context, tx Transaction, values []IndexValue) "
                   "([]VersionedRecord, error) {\n";
            out << "\treturn repository.ListByIndexTx(ctx, tx, " << go_string(index.name)
                << ", values)\n";
            out << "}\n\n";
        }
        out << "func (Default" << type_name
            << "Repository) UpdateTx(ctx context.Context, tx Transaction, keyValues "
               "[]EntityKeyValue, document JSON, expectedVersion Version) (*VersionedRecord, "
               "error) {\n";
        out << "\treturn DefaultEntityRepository{}.UpsertEntityTx(ctx, tx, EntityUpsertRequest{\n";
        out << "\t\tLookup: " << type_name << "Lookup(keyValues),\n";
        out << "\t\tDocument: document,\n";
        out << "\t\tExpectedVersion: &expectedVersion,\n";
        out << "\t})\n";
        out << "}\n\n";
        out << "func " << type_name << "CollectionDescriptor() CollectionDescriptor {\n";
        out << "\treturn CollectionDescriptor{\n";
        out << "\t\tName: " << go_string(entity.name) << ",\n";
        out << "\t\tFields: []FieldDescriptor{\n";
        for (const auto& field : entity.fields)
        {
            out << "\t\t\t" << go_field_descriptor_expr(field) << ",\n";
        }
        out << "\t\t},\n";
        out << "\t\tKeyFields: []string{";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(entity.key_fields[i]);
        }
        out << "},\n";
        out << "\t\tIndexes: []IndexDescriptor{\n";
        for (const auto& index : entity.indexes)
        {
            out << "\t\t\t{Name: " << go_string(index.name) << ", Fields: []string{";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_string(index.fields[i]);
            }
            out << "}, Unique: " << (index.unique ? "true" : "false") << "},\n";
        }
        out << "\t\t},\n";
        out << "\t\tSchemaVersion: 1,\n";
        out << "\t}\n";
        out << "}\n\n";
        out << "func " << type_name << "EntityDescriptor() EntityDescriptor {\n";
        out << "\tfor _, descriptor := range EntityDescriptors() {\n";
        out << "\t\tif descriptor.Name == " << go_string(entity.name) << " {\n";
        out << "\t\t\treturn descriptor\n";
        out << "\t\t}\n";
        out << "\t}\n";
        out << "\tpanic(\"entity descriptor not found: " << entity.name << "\")\n";
        out << "}\n\n";
        out << "var _ " << type_name << "Repository = Default" << type_name << "Repository{}\n\n";
    }

    return out.str();
}

} // namespace statespec
