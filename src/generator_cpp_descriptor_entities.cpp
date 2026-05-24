#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"
#include "generator_entity_index_helpers.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_entity_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& entity : system.entities)
    {
        out << "inline constexpr const char* " << cpp_entity_name_constant_name(entity.name)
            << " = " << cpp_string(entity.name) << ";\n";
        for (const auto& field : entity.fields)
        {
            out << "inline constexpr const char* "
                << cpp_entity_field_constant_name(entity.name, field.name) << " = "
                << cpp_string(field.name) << ";\n";
        }
        for (const auto& index : entity.indexes)
        {
            out << "inline constexpr const char* "
                << cpp_entity_index_constant_name(entity.name, index.name) << " = "
                << cpp_string(index.name) << ";\n";
        }
        for (const auto& state : entity.states)
        {
            out << "inline constexpr const char* "
                << cpp_entity_state_constant_name(entity.name, state.name) << " = "
                << cpp_string(state.name) << ";\n";
        }
    }
    if (!system.entities.empty())
    {
        out << "\n";
    }

    out << "inline std::vector<EntityDescriptor> entity_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& entity : system.entities)
    {
        out << "        EntityDescriptor{\n";
        out << "            " << cpp_entity_name_constant_name(entity.name) << ",\n";
        out << "            {";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_entity_field_constant_name(entity.name, entity.key_fields[i]);
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
            out << "                    " << cpp_entity_state_constant_name(entity.name, state.name)
                << ",\n";
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
        if (entity.initial_state.has_value())
        {
            out << "            std::optional<std::string>{"
                << cpp_entity_state_constant_name(entity.name, *entity.initial_state) << "},\n";
        }
        else
        {
            out << "            std::nullopt,\n";
        }
        out << "            {";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_entity_state_constant_name(entity.name, entity.terminal_states[i]);
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
        out << "            " << cpp_entity_name_constant_name(entity.name) << ",\n";
        out << "            {\n";
        for (const auto& field : entity.fields)
        {
            out << "                " << cpp_entity_field_descriptor_expr(entity.name, field)
                << ",\n";
        }
        out << "            },\n";
        out << "            {";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "},\n";
        out << "            {\n";
        for (const auto& index : entity.indexes)
        {
            out << "                statespec::backend::IndexDescriptor{\n";
            out << "                    " << cpp_entity_index_constant_name(entity.name, index.name)
                << ",\n";
            out << "                    {";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << cpp_entity_field_constant_name(entity.name, index.fields[i]);
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

    for (const auto& entity : system.entities)
    {
        const auto type_name = pascal_identifier(entity.name);
        const auto function_prefix = snake_identifier(entity.name);
        out << "inline EntityLookup " << function_prefix
            << "_lookup(std::vector<EntityKeyValue> key_values)\n";
        out << "{\n";
        out << "    return EntityLookup{\n";
        out << "        " << cpp_entity_name_constant_name(entity.name) << ",\n";
        out << "        {";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "},\n";
        out << "        std::move(key_values),\n";
        out << "    };\n";
        out << "}\n\n";
        out << "class I" << type_name << "Repository\n";
        out << "{\n";
        out << "  public:\n";
        out << "    virtual ~I" << type_name << "Repository() = default;\n";
        out << "    virtual void register_descriptor(statespec::backend::IBackend& backend) = 0;\n";
        out << "    virtual std::optional<statespec::backend::VersionedRecord> createTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        statespec::backend::Json document\n";
        out << "    ) = 0;\n";
        out << "    virtual std::optional<statespec::backend::VersionedRecord> getTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::vector<EntityKeyValue> key_values\n";
        out << "    ) = 0;\n";
        out << "    virtual std::vector<statespec::backend::VersionedRecord> listByIndexTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::string index_name,\n";
        out << "        std::vector<statespec::backend::IndexValue> values\n";
        out << "    ) = 0;\n";
        for (const auto& index : entity.indexes)
        {
            out << "    virtual std::vector<statespec::backend::VersionedRecord> "
                << entity_index_repository_method_name(index.name) << "(\n";
            out << "        statespec::backend::ITransaction& tx,\n";
            out << "        std::vector<statespec::backend::IndexValue> values\n";
            out << "    ) = 0;\n";
        }
        out << "    virtual std::optional<statespec::backend::VersionedRecord> updateTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::vector<EntityKeyValue> key_values,\n";
        out << "        statespec::backend::Json document,\n";
        out << "        statespec::backend::Version expected_version\n";
        out << "    ) = 0;\n";
        out << "};\n\n";
        out << "class Default" << type_name << "Repository final : public I" << type_name
            << "Repository\n";
        out << "{\n";
        out << "  public:\n";
        out << "    void register_descriptor(statespec::backend::IBackend& backend) override\n";
        out << "    {\n";
        out << "        backend.ensure_collection(collection_descriptor());\n";
        out << "    }\n\n";
        out << "    std::optional<statespec::backend::VersionedRecord> createTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        statespec::backend::Json document\n";
        out << "    ) override\n";
        out << "    {\n";
        out << "        return entities_.create_entityTx(\n";
        out << "            tx,\n";
        out << "            EntityCreateRequest{\n";
        out << "                entity_lookup_from_document(entity_descriptor(), document),\n";
        out << "                std::move(document),\n";
        out << "            }\n";
        out << "        );\n";
        out << "    }\n\n";
        out << "    std::optional<statespec::backend::VersionedRecord> getTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::vector<EntityKeyValue> key_values\n";
        out << "    ) override\n";
        out << "    {\n";
        out << "        return entities_.get_entityTx(\n";
        out << "            tx, EntityGetRequest{" << function_prefix
            << "_lookup(std::move(key_values))}\n";
        out << "        );\n";
        out << "    }\n\n";
        out << "    std::vector<statespec::backend::VersionedRecord> listByIndexTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::string index_name,\n";
        out << "        std::vector<statespec::backend::IndexValue> values\n";
        out << "    ) override\n";
        out << "    {\n";
        out << "        return entities_.list_entities_by_indexTx(\n";
        out << "            tx,\n";
        out << "            EntityListByIndexRequest{\n";
        out << "                " << cpp_entity_name_constant_name(entity.name) << ",\n";
        out << "                std::move(index_name),\n";
        out << "                std::move(values),\n";
        out << "            }\n";
        out << "        );\n";
        out << "    }\n\n";
        for (const auto& index : entity.indexes)
        {
            out << "    std::vector<statespec::backend::VersionedRecord> "
                << entity_index_repository_method_name(index.name) << "(\n";
            out << "        statespec::backend::ITransaction& tx,\n";
            out << "        std::vector<statespec::backend::IndexValue> values\n";
            out << "    ) override\n";
            out << "    {\n";
            out << "        return listByIndexTx(tx, "
                << cpp_entity_index_constant_name(entity.name, index.name)
                << ", std::move(values));\n";
            out << "    }\n\n";
        }
        out << "    std::optional<statespec::backend::VersionedRecord> updateTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::vector<EntityKeyValue> key_values,\n";
        out << "        statespec::backend::Json document,\n";
        out << "        statespec::backend::Version expected_version\n";
        out << "    ) override\n";
        out << "    {\n";
        out << "        return entities_.upsert_entityTx(\n";
        out << "            tx,\n";
        out << "            EntityUpsertRequest{\n";
        out << "                " << function_prefix << "_lookup(std::move(key_values)),\n";
        out << "                std::move(document),\n";
        out << "                expected_version,\n";
        out << "            }\n";
        out << "        );\n";
        out << "    }\n\n";
        out << "    static statespec::backend::CollectionDescriptor collection_descriptor()\n";
        out << "    {\n";
        out << "        return statespec::backend::CollectionDescriptor{\n";
        out << "            " << cpp_entity_name_constant_name(entity.name) << ",\n";
        out << "            {\n";
        for (const auto& field : entity.fields)
        {
            out << "                " << cpp_entity_field_descriptor_expr(entity.name, field)
                << ",\n";
        }
        out << "            },\n";
        out << "            {";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "},\n";
        out << "            {\n";
        for (const auto& index : entity.indexes)
        {
            out << "                statespec::backend::IndexDescriptor{\n";
            out << "                    " << cpp_entity_index_constant_name(entity.name, index.name)
                << ",\n";
            out << "                    {";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << cpp_entity_field_constant_name(entity.name, index.fields[i]);
            }
            out << "},\n";
            out << "                    " << (index.unique ? "true" : "false") << ",\n";
            out << "                },\n";
        }
        out << "            },\n";
        out << "            1,\n";
        out << "        };\n";
        out << "    }\n\n";
        out << "    static EntityDescriptor entity_descriptor()\n";
        out << "    {\n";
        out << "        for (const auto& descriptor : entity_descriptors())\n";
        out << "        {\n";
        out << "            if (descriptor.name == " << cpp_entity_name_constant_name(entity.name)
            << ")\n";
        out << "            {\n";
        out << "                return descriptor;\n";
        out << "            }\n";
        out << "        }\n";
        out << "        throw statespec::backend::BackendError(std::string{\"entity descriptor "
               "not found: \"} + "
            << cpp_entity_name_constant_name(entity.name) << ");\n";
        out << "    }\n\n";
        out << "  private:\n";
        out << "    DefaultEntityRepository entities_;\n";
        out << "};\n\n";
    }

    return out.str();
}

} // namespace statespec
