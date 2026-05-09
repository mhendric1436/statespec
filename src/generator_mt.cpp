#include "generator_backend.hpp"

namespace statespec::generator_backend
{

namespace
{

std::string generate_mt_manifest(
    const SystemDecl& system,
    const GenerateDecl& declaration
)
{
    std::ostringstream out;
    append_header(out, system, declaration.target);
    out << "entities:\n";
    for (const auto& entity : system.entities)
    {
        out << "  - name: " << entity.name << "\n";
        out << "    keys:\n";
        for (const auto& key : entity.key_fields)
        {
            out << "      - " << key << "\n";
        }
        out << "    fields:\n";
        for (const auto& field : entity.fields)
        {
            out << "      - name: " << field.name << "\n";
            out << "        type: " << field.type << "\n";
            out << "        cpp_type: " << cpp_type_for_statespec_type(field.type) << "\n";
            out << "        optional: " << bool_text(is_optional_type(field.type)) << "\n";
        }
    }
    return out.str();
}

std::string generate_mt_entities_header(const SystemDecl& system)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include <chrono>\n";
    out << "#include <cstddef>\n";
    out << "#include <cstdint>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n";
    out << "#include <string_view>\n\n";
    out << "namespace statespec_generated::mt\n";
    out << "{\n\n";
    out << "struct FieldMetadata\n";
    out << "{\n";
    out << "    std::string_view name;\n";
    out << "    std::string_view statespec_type;\n";
    out << "    std::string_view cpp_type;\n";
    out << "    bool optional;\n";
    out << "};\n\n";
    out << "struct EntityMetadata\n";
    out << "{\n";
    out << "    std::string_view name;\n";
    out << "    const FieldMetadata* fields;\n";
    out << "    std::size_t field_count;\n";
    out << "    const std::string_view* keys;\n";
    out << "    std::size_t key_count;\n";
    out << "};\n\n";

    for (const auto& entity : system.entities)
    {
        out << "struct " << entity.name << "\n";
        out << "{\n";
        for (const auto& field : entity.fields)
        {
            out << "    " << cpp_type_for_statespec_type(field.type) << ' ' << field.name
                << "{};\n";
        }
        out << "};\n\n";
    }

    out << "const EntityMetadata* entities();\n";
    out << "std::size_t entity_count();\n\n";
    out << "} // namespace statespec_generated::mt\n";
    return out.str();
}

std::string generate_mt_metadata_source(const SystemDecl& system)
{
    std::ostringstream out;
    out << "#include \"mt_entities.hpp\"\n\n";
    out << "#include <array>\n\n";
    out << "namespace statespec_generated::mt\n";
    out << "{\n";
    out << "namespace\n";
    out << "{\n\n";

    for (const auto& entity : system.entities)
    {
        const auto symbol = to_lower(entity.name);
        out << "constexpr std::array<FieldMetadata, " << entity.fields.size() << "> " << symbol
            << "_fields{{\n";
        for (const auto& field : entity.fields)
        {
            out << "    FieldMetadata{\"" << field.name << "\", \"" << field.type << "\", \""
                << cpp_type_for_statespec_type(field.type) << "\", "
                << bool_text(is_optional_type(field.type)) << "},\n";
        }
        out << "}};\n\n";
        out << "constexpr std::array<std::string_view, " << entity.key_fields.size() << "> "
            << symbol << "_keys{{\n";
        for (const auto& key : entity.key_fields)
        {
            out << "    \"" << key << "\",\n";
        }
        out << "}};\n\n";
    }

    out << "constexpr std::array<EntityMetadata, " << system.entities.size()
        << "> all_entities{{\n";
    for (const auto& entity : system.entities)
    {
        const auto symbol = to_lower(entity.name);
        out << "    EntityMetadata{\"" << entity.name << "\", " << symbol << "_fields.data(), "
            << symbol << "_fields.size(), " << symbol << "_keys.data(), " << symbol
            << "_keys.size()},\n";
    }
    out << "}};\n\n";
    out << "} // namespace\n\n";
    out << "const EntityMetadata* entities()\n";
    out << "{\n";
    out << "    return all_entities.data();\n";
    out << "}\n\n";
    out << "std::size_t entity_count()\n";
    out << "{\n";
    out << "    return all_entities.size();\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::mt\n";
    return out.str();
}

std::string generate_mt_state_machine_manifest(const SystemDecl& system)
{
    std::ostringstream out;
    append_header(out, system, "mt-state-machines");
    out << "state_machines:\n";
    for (const auto& entity : system.entities)
    {
        if (!entity.state_machine.has_value())
        {
            continue;
        }
        out << "  - entity: " << entity.name << "\n";
        out << "    initial: " << optional_or_empty(entity.state_machine->initial_state) << "\n";
        out << "    states:\n";
        for (const auto& state : entity.state_machine->states)
        {
            out << "      - " << state.name << "\n";
        }
        out << "    terminal_states:\n";
        for (const auto& state : entity.state_machine->terminal_states)
        {
            out << "      - " << state << "\n";
        }
        out << "    transitions:\n";
        for (const auto& transition : entity.state_machine->transitions)
        {
            out << "      - from: " << transition.from << "\n";
            out << "        to: " << transition.to << "\n";
        }
    }
    return out.str();
}

} // namespace

void generate_mt(
    const SystemDecl& system,
    const GenerateDecl& declaration,
    const GenerationOptions& options,
    GenerationResult& result
)
{
    const auto root = output_root(declaration, options);
    result.files.push_back(
        GeneratedFile{
            join_path(root, "mt-manifest.yaml"), generate_mt_manifest(system, declaration)
        }
    );
    result.files.push_back(
        GeneratedFile{join_path(root, "mt_entities.hpp"), generate_mt_entities_header(system)}
    );
    result.files.push_back(
        GeneratedFile{join_path(root, "mt_metadata.cpp"), generate_mt_metadata_source(system)}
    );
    result.files.push_back(
        GeneratedFile{
            join_path(root, "mt-state-machines.yaml"), generate_mt_state_machine_manifest(system)
        }
    );
}

} // namespace statespec::generator_backend
