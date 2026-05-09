#include "generator_backend.hpp"

#include <cctype>
#include <string_view>
#include <unordered_set>

namespace statespec::generator_backend
{

namespace
{

std::string json_escape(std::string_view value)
{
    std::ostringstream out;
    for (const auto ch : value)
    {
        switch (ch)
        {
        case '\\':
            out << "\\\\";
            break;
        case '"':
            out << "\\\"";
            break;
        case '\b':
            out << "\\b";
            break;
        case '\f':
            out << "\\f";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            out << ch;
            break;
        }
    }
    return out.str();
}

std::string json_string(std::string_view value)
{
    return "\"" + json_escape(value) + "\"";
}

std::string mt_identifier_from_name(std::string_view value)
{
    std::string identifier;
    char previous = '\0';
    for (const auto raw_ch : value)
    {
        const auto ch = static_cast<unsigned char>(raw_ch);
        if (std::isalnum(ch))
        {
            if (std::isupper(ch) && !identifier.empty() && identifier.back() != '_' &&
                (std::islower(static_cast<unsigned char>(previous)) ||
                 std::isdigit(static_cast<unsigned char>(previous))))
            {
                identifier.push_back('_');
            }
            identifier.push_back(static_cast<char>(std::tolower(ch)));
        }
        else if (raw_ch == '_' && !identifier.empty() && identifier.back() != '_')
        {
            identifier.push_back('_');
        }
        previous = raw_ch;
    }

    if (identifier.empty() || !std::isalpha(static_cast<unsigned char>(identifier.front())))
    {
        identifier = "t_" + identifier;
    }
    if (identifier.rfind("mt_", 0) == 0)
    {
        identifier = "ss_" + identifier;
    }
    if (identifier.size() > 39)
    {
        identifier.resize(39);
    }
    while (!identifier.empty() && identifier.back() == '_')
    {
        identifier.pop_back();
    }
    return identifier.empty() ? "table" : identifier;
}

std::string mt_filename_from_entity_name(std::string_view value)
{
    return mt_identifier_from_name(value) + ".mt.json";
}

std::string mt_value_type_for_statespec_type(const std::string& type)
{
    const auto base = strip_optional_suffix(type);
    if (base == "bool")
    {
        return "bool";
    }
    if (base == "int" || base == "int32" || base == "int64" || base == "long")
    {
        return "int64";
    }
    if (base == "double" || base == "decimal")
    {
        return "double";
    }
    if (base == "json")
    {
        return "json";
    }
    if (base == "string" || base == "uuid" || base == "timestamp" || base == "duration")
    {
        return "string";
    }
    return "json";
}

bool contains_key_field(
    const EntityDecl& entity,
    const std::string& field_name
)
{
    return std::find(entity.key_fields.begin(), entity.key_fields.end(), field_name) !=
           entity.key_fields.end();
}

std::string generate_mt_codegen_schema(const EntityDecl& entity)
{
    std::ostringstream out;
    out << "{\n";
    out << "  \"namespace\": \"statespec_generated::mt\",\n";
    out << "  \"table_name\": " << json_string(mt_identifier_from_name(entity.name)) << ",\n";
    out << "  \"class_name\": " << json_string(entity.name) << ",\n";
    out << "  \"key\": ";
    if (entity.key_fields.size() == 1)
    {
        out << json_string(entity.key_fields.front()) << ",\n";
    }
    else
    {
        out << "{\n";
        out << "    \"fields\": [";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << json_string(entity.key_fields[i]);
        }
        out << "],\n";
        out << "    \"separator\": \":\"\n";
        out << "  },\n";
    }
    out << "  \"schema_version\": 1,\n";
    out << "  \"fields\": [\n";
    for (std::size_t i = 0; i < entity.fields.size(); ++i)
    {
        const auto& field = entity.fields[i];
        const auto optional = is_optional_type(field.type);
        out << "    {\"name\": " << json_string(field.name) << ", ";
        if (optional)
        {
            out << "\"type\": \"optional\", \"value_type\": "
                << json_string(mt_value_type_for_statespec_type(field.type));
        }
        else
        {
            out << "\"type\": " << json_string(mt_value_type_for_statespec_type(field.type));
            if (contains_key_field(entity, field.name))
            {
                out << ", \"required\": true";
            }
            else
            {
                out << ", \"required\": true";
            }
        }
        out << "}";
        if (i + 1 < entity.fields.size())
        {
            out << ",";
        }
        out << "\n";
    }
    out << "  ],\n";
    out << "  \"indexes\": []\n";
    out << "}\n";
    return out.str();
}

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
        out << "    codegen_schema: schemas/" << mt_filename_from_entity_name(entity.name) << "\n";
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
    for (const auto& entity : system.entities)
    {
        result.files.push_back(
            GeneratedFile{
                join_path(join_path(root, "schemas"), mt_filename_from_entity_name(entity.name)),
                generate_mt_codegen_schema(entity)
            }
        );
    }
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
