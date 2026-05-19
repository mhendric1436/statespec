#include "statespec/generator_cpp.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace statespec
{

namespace
{

std::string read_template_file(
    const std::filesystem::path& path,
    DiagnosticBag& diagnostics
)
{
    std::ifstream input(path);
    if (!input)
    {
        diagnostics.error(
            SourceRange{}, "SSPEC5201", "failed to read binding template: " + path.string()
        );
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void add_template_file(
    GenerationResult& result,
    const std::filesystem::path& output_dir,
    const std::filesystem::path& template_path,
    const std::filesystem::path& relative_output_path,
    DiagnosticBag& diagnostics,
    GeneratedArtifactTier tier = GeneratedArtifactTier::Common
)
{
    const auto content = read_template_file(template_path, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }

    result.files.push_back(
        GeneratedFile{
            (output_dir / "common" / relative_output_path).string(),
            content,
            tier,
            (std::filesystem::path{"common"} / relative_output_path).generic_string(),
        }
    );
}

std::string cpp_string(const std::string& value)
{
    std::ostringstream out;
    out << '"';
    for (const auto ch : value)
    {
        switch (ch)
        {
        case '"':
            out << "\\\"";
            break;
        case '\\':
            out << "\\\\";
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
    out << '"';
    return out.str();
}

bool is_optional_type(const std::string& type)
{
    return !type.empty() && type.back() == '?';
}

std::string strip_optional_suffix(const std::string& type)
{
    return is_optional_type(type) ? type.substr(0, type.size() - 1) : type;
}

std::string cpp_field_type_expr(FieldDescriptorType type)
{
    switch (type)
    {
    case FieldDescriptorType::String:
        return "statespec::backend::FieldType::String";
    case FieldDescriptorType::Bool:
        return "statespec::backend::FieldType::Bool";
    case FieldDescriptorType::Int:
        return "statespec::backend::FieldType::Int";
    case FieldDescriptorType::Int32:
        return "statespec::backend::FieldType::Int32";
    case FieldDescriptorType::Int64:
        return "statespec::backend::FieldType::Int64";
    case FieldDescriptorType::Long:
        return "statespec::backend::FieldType::Long";
    case FieldDescriptorType::Double:
        return "statespec::backend::FieldType::Double";
    case FieldDescriptorType::Decimal:
        return "statespec::backend::FieldType::Decimal";
    case FieldDescriptorType::Json:
        return "statespec::backend::FieldType::Json";
    case FieldDescriptorType::Timestamp:
        return "statespec::backend::FieldType::Timestamp";
    case FieldDescriptorType::Duration:
        return "statespec::backend::FieldType::Duration";
    case FieldDescriptorType::Uuid:
        return "statespec::backend::FieldType::Uuid";
    case FieldDescriptorType::Named:
        return "statespec::backend::FieldType::Named";
    case FieldDescriptorType::List:
        return "statespec::backend::FieldType::List";
    case FieldDescriptorType::Set:
        return "statespec::backend::FieldType::Set";
    case FieldDescriptorType::Map:
        return "statespec::backend::FieldType::Map";
    case FieldDescriptorType::Optional:
        return "statespec::backend::FieldType::Optional";
    case FieldDescriptorType::Reference:
        return "statespec::backend::FieldType::Reference";
    }
    return "statespec::backend::FieldType::Named";
}

bool is_required_descriptor_field(const std::string& type)
{
    return classify_field_descriptor_type(type) != FieldDescriptorType::Optional;
}

std::string cpp_field_descriptor_expr(const IrField& field)
{
    return "statespec::backend::FieldDescriptor{" + cpp_string(field.name) + ", " +
           cpp_field_type_expr(classify_field_descriptor_type(field.type)) + ", " +
           cpp_string(field.type) + ", " +
           (is_required_descriptor_field(field.type) ? "true" : "false") + "}";
}

std::string pascal_identifier(const std::string& value)
{
    std::string result;
    bool upper_next = true;
    for (const auto ch : value)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0)
        {
            upper_next = true;
            continue;
        }
        result.push_back(
            upper_next ? static_cast<char>(std::toupper(static_cast<unsigned char>(ch))) : ch
        );
        upper_next = false;
    }
    return result.empty() ? "GeneratedShape" : result;
}

std::string snake_identifier(const std::string& value)
{
    std::string result;
    bool previous_was_separator = true;
    for (const auto ch : value)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0)
        {
            if (!result.empty() && !previous_was_separator)
            {
                result.push_back('_');
            }
            previous_was_separator = true;
            continue;
        }
        if (std::isupper(static_cast<unsigned char>(ch)) != 0 && !previous_was_separator &&
            !result.empty())
        {
            result.push_back('_');
        }
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        previous_was_separator = false;
    }
    return result.empty() ? "generated_event" : result;
}

std::string generate_makefile(BindingGenerationTier tier)
{
    std::ostringstream out;
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
    const auto include_worker =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker;

    out << "CXX ?= clang++\n";
    out << "CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -I. -Icommon\n";
    out << "BUILD_DIR ?= build\n\n";
    out << "CHECK_TARGETS := check-common";
    if (include_api)
    {
        out << " check-api";
    }
    if (include_worker)
    {
        out << " check-worker";
    }
    out << "\n\n";
    out << ".PHONY: all check check-common";
    if (include_api)
    {
        out << " check-api";
    }
    if (include_worker)
    {
        out << " check-worker";
    }
    out << " clean\n\n";
    out << "all: check\n\n";
    out << "check: $(CHECK_TARGETS)\n\n";
    out << "$(BUILD_DIR):\n";
    out << "\tmkdir -p $(BUILD_DIR)\n\n";
    out << "check-common: $(BUILD_DIR)\n";
    out << "\tprintf '#include \"common/system_descriptors.hpp\"\\nint main() { return 0; }\\n' | "
           "$(CXX) $(CXXFLAGS) -x c++ - -o $(BUILD_DIR)/check-common\n\n";
    if (include_api)
    {
        out << "check-api: $(BUILD_DIR)\n";
        out << "\tprintf '#include \"api_artifacts.hpp\"\\nint main() { return 0; }\\n' | "
               "$(CXX) $(CXXFLAGS) -x c++ - -o $(BUILD_DIR)/check-api\n\n";
    }
    if (include_worker)
    {
        out << "check-worker: $(BUILD_DIR)\n";
        out << "\tprintf '#include \"worker_artifacts.hpp\"\\nint main() { return 0; }\\n' | "
               "$(CXX) $(CXXFLAGS) -x c++ - -o $(BUILD_DIR)/check-worker\n\n";
    }
    out << "clean:\n";
    out << "\trm -rf $(BUILD_DIR)\n";
    return out.str();
}

std::string cpp_shape_type(const std::string& type)
{
    const auto optional = is_optional_type(type);
    const auto base = strip_optional_suffix(type);
    std::string mapped = "std::string";
    if (base == "bool")
    {
        mapped = "bool";
    }
    else if (base == "int" || base == "int32")
    {
        mapped = "std::int32_t";
    }
    else if (base == "int64" || base == "long")
    {
        mapped = "std::int64_t";
    }
    else if (base == "double" || base == "decimal")
    {
        mapped = "double";
    }

    return optional ? "std::optional<" + mapped + ">" : mapped;
}

std::int64_t parse_duration_seconds(const std::optional<std::string>& value)
{
    if (!value.has_value() || value->empty())
    {
        return 0;
    }

    const auto& text = *value;
    if (text.size() >= 3 && text[0] == 'P' && text[1] == 'T')
    {
        std::int64_t total = 0;
        std::int64_t current = 0;
        for (std::size_t i = 2; i < text.size(); ++i)
        {
            const auto ch = text[i];
            if (std::isdigit(static_cast<unsigned char>(ch)))
            {
                current = current * 10 + static_cast<std::int64_t>(ch - '0');
            }
            else
            {
                if (ch == 'H')
                {
                    total += current * 3600;
                }
                else if (ch == 'M')
                {
                    total += current * 60;
                }
                else if (ch == 'S')
                {
                    total += current;
                }
                current = 0;
            }
        }
        return total;
    }

    return 0;
}

std::string optional_string_expr(const std::optional<std::string>& value)
{
    if (!value.has_value())
    {
        return "std::nullopt";
    }
    return "std::optional<std::string>{" + cpp_string(*value) + "}";
}

const IrApi* find_api(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& api : system.apis)
    {
        if (api.name == name)
        {
            return &api;
        }
    }
    return nullptr;
}

const std::optional<std::string>& optional_api_field(
    const IrApi* api,
    const std::optional<std::string> IrApi::* field
)
{
    static const std::optional<std::string> empty;
    return api == nullptr ? empty : api->*field;
}

std::string generate_system_descriptors_header(const IrSystem& system)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"backend.hpp\"\n";
    out << "#include \"external_system.hpp\"\n";
    out << "#include \"feature_flag.hpp\"\n";
    out << "#include \"lease.hpp\"\n";
    out << "#include \"log.hpp\"\n";
    out << "#include \"metric.hpp\"\n";
    out << "#include \"queue.hpp\"\n";
    out << "#include \"workflow.hpp\"\n\n";
    out << "#include <chrono>\n";
    out << "#include <cstdint>\n";
    out << "#include <map>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n";
    out << "#include <string_view>\n";
    out << "#include <utility>\n";
    out << "#include <vector>\n\n";
    out << "namespace statespec_generated\n";
    out << "{\n\n";
    out << "struct EventEnvelope\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::map<std::string, statespec::backend::Json> fields;\n";
    out << "};\n\n";
    for (const auto& shape : system.shapes)
    {
        out << "struct " << pascal_identifier(shape.name) << "\n";
        out << "{\n";
        for (const auto& field : shape.fields)
        {
            out << "    " << cpp_shape_type(field.type) << " " << field.name << "{};\n";
        }
        out << "};\n\n";
    }

    for (const auto& event : system.events)
    {
        out << "inline EventEnvelope make_" << snake_identifier(event.name) << "_event(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "    statespec::backend::Json " << field.name;
            out << (i + 1 < event.fields.size() ? ",\n" : "\n");
        }
        out << ")\n";
        out << "{\n";
        out << "    return EventEnvelope{\n";
        out << "        " << cpp_string(event.name) << ",\n";
        out << "        {\n";
        for (const auto& field : event.fields)
        {
            out << "            {" << cpp_string(field.name) << ", std::move(" << field.name
                << ")},\n";
        }
        out << "        },\n";
        out << "    };\n";
        out << "}\n\n";
    }

    out << "struct LeaseDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> resource;\n";
    out << "    std::chrono::seconds ttl;\n";
    out << "    std::optional<std::chrono::seconds> renew_every;\n";
    out << "    std::optional<std::string> holder;\n";
    out << "    bool fencing_token = false;\n";
    out << "    std::optional<std::chrono::seconds> max_ttl;\n";
    out << "};\n\n";
    out << "struct FeatureFlagDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string type;\n";
    out << "    std::string default_value;\n";
    out << "    std::string scope;\n";
    out << "    std::optional<std::string> owner;\n";
    out << "    std::optional<std::string> description;\n";
    out << "    std::optional<std::string> expires;\n";
    out << "};\n\n";

    out << "struct ValueDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string type;\n";
    out << "    std::optional<std::string> constraint;\n";
    out << "};\n\n";

    out << "struct EnumMemberDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> value;\n";
    out << "};\n\n";

    out << "struct EnumDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<EnumMemberDescriptor> members;\n";
    out << "};\n\n";

    out << "struct EventDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> fields;\n";
    out << "};\n\n";

    out << "struct ExternalSystemPropertyDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string value;\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMappingDescriptor\n";
    out << "{\n";
    out << "    std::string source;\n";
    out << "    std::string target;\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMappingAssignment\n";
    out << "{\n";
    out << "    std::string source;\n";
    out << "    std::string source_root;\n";
    out << "    std::string source_field;\n";
    out << "    std::string target_root;\n";
    out << "    std::string field;\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMappingPlan\n";
    out << "{\n";
    out << "    std::vector<ExternalSystemMetadataMappingAssignment> all_mappings;\n";
    out << "    std::vector<ExternalSystemMetadataMappingAssignment> client_mappings;\n";
    out << "    std::vector<ExternalSystemMetadataMappingAssignment> request_mappings;\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMissingMappingSource\n";
    out << "{\n";
    out << "    std::string source;\n";
    out << "    std::string source_root;\n";
    out << "    std::string source_field;\n";
    out << "    std::string target_root;\n";
    out << "    std::string field;\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMappingInputs\n";
    out << "{\n";
    out << "    std::map<std::string, statespec::backend::Json> input;\n";
    out << "    std::map<std::string, statespec::backend::Json> entity;\n";
    out << "    std::map<std::string, statespec::backend::Json> workflow;\n";
    out << "    std::map<std::string, statespec::backend::Json> metadata;\n";
    out << "\n";
    out << "    const statespec::backend::Json* source_value(\n";
    out << "        std::string_view source_root,\n";
    out << "        std::string_view source_field\n";
    out << "    ) const\n";
    out << "    {\n";
    out << "        const auto* values = source_root == \"input\" ? &input\n";
    out << "            : source_root == \"entity\"                 ? &entity\n";
    out << "            : source_root == \"workflow\"               ? &workflow\n";
    out << "            : source_root == \"metadata\"               ? &metadata\n";
    out << "                                                       : nullptr;\n";
    out << "        if (values == nullptr)\n";
    out << "        {\n";
    out << "            return nullptr;\n";
    out << "        }\n";
    out << "        const auto value = values->find(std::string(source_field));\n";
    out << "        return value == values->end() ? nullptr : &value->second;\n";
    out << "    }\n";
    out << "\n";
    out << "    const statespec::backend::Json* source_value(\n";
    out << "        const ExternalSystemMetadataMappingAssignment& assignment\n";
    out << "    ) const\n";
    out << "    {\n";
    out << "        return source_value(assignment.source_root, assignment.source_field);\n";
    out << "    }\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMappingOutput\n";
    out << "{\n";
    out << "    std::map<std::string, statespec::backend::Json> client_config;\n";
    out << "    std::map<std::string, statespec::backend::Json> request_payload;\n";
    out << "    std::vector<ExternalSystemMetadataMissingMappingSource> missing_sources;\n";
    out << "};\n\n";

    out << "inline std::vector<ExternalSystemMetadataMissingMappingSource> "
           "missing_external_system_metadata_mapping_sources(\n";
    out << "    const ExternalSystemMetadataMappingPlan& plan,\n";
    out << "    const ExternalSystemMetadataMappingInputs& inputs\n";
    out << ")\n";
    out << "{\n";
    out << "    std::vector<ExternalSystemMetadataMissingMappingSource> missing;\n";
    out << "    for (const auto& assignment : plan.all_mappings)\n";
    out << "    {\n";
    out << "        if (inputs.source_value(assignment) == nullptr)\n";
    out << "        {\n";
    out << "            missing.push_back(ExternalSystemMetadataMissingMappingSource{\n";
    out << "                assignment.source,\n";
    out << "                assignment.source_root,\n";
    out << "                assignment.source_field,\n";
    out << "                assignment.target_root,\n";
    out << "                assignment.field,\n";
    out << "            });\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return missing;\n";
    out << "}\n\n";

    out << "class IExternalSystemMetadataMappingApplicator\n";
    out << "{\n";
    out << "public:\n";
    out << "    virtual ~IExternalSystemMetadataMappingApplicator() = default;\n";
    out << "    virtual ExternalSystemMetadataMappingOutput apply(\n";
    out << "        const ExternalSystemMetadataMappingPlan& plan,\n";
    out << "        const ExternalSystemMetadataMappingInputs& inputs\n";
    out << "    ) = 0;\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataDescriptor\n";
    out << "{\n";
    out << "    std::string entity;\n";
    out << "    std::optional<std::string> tenant_field;\n";
    out << "    std::string profile_field;\n";
    out << "    std::vector<std::string> key_fields;\n";
    out << "    std::vector<std::string> required_fields;\n";
    out << "    std::vector<ExternalSystemMetadataMappingDescriptor> mappings;\n";
    out << "};\n\n";

    out << "struct ExternalSystemOperatorMetadataUpsertRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "    statespec::backend::Json document;\n";
    out << "    std::optional<statespec::backend::Version> expected_version;\n";
    out << "};\n\n";

    out << "struct ExternalSystemOperatorMetadataGetRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "};\n\n";

    out << "struct ExternalSystemOperatorMetadataDisableRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "    std::optional<statespec::backend::Version> expected_version;\n";
    out << "    std::string disabled_status = \"Disabled\";\n";
    out << "};\n\n";

    out << "struct ExternalSystemOperatorMetadataDeleteRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "    std::optional<statespec::backend::Version> expected_version;\n";
    out << "    std::string deleted_status = \"Deleted\";\n";
    out << "};\n\n";

    out << "class IExternalSystemOperatorMetadataRepository\n";
    out << "{\n";
    out << "public:\n";
    out << "    virtual ~IExternalSystemOperatorMetadataRepository() = default;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> upsert_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataUpsertRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> get_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataGetRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> disable_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataDisableRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> delete_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataDeleteRequest& request\n";
    out << "    ) = 0;\n";
    out << "};\n\n";

    out << "struct ExternalSystemDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<ExternalSystemPropertyDescriptor> properties;\n";
    out << "    std::optional<ExternalSystemMetadataDescriptor> metadata;\n";
    out << "};\n\n";

    out << "struct ApiDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> method;\n";
    out << "    std::optional<std::string> path;\n";
    out << "    std::optional<std::string> input;\n";
    out << "    std::optional<std::string> output;\n";
    out << "    std::optional<std::string> error;\n";
    out << "    std::optional<std::string> starts_workflow;\n";
    out << "    std::optional<std::string> enqueues;\n";
    out << "};\n\n";

    out << "struct ApiServerDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<std::string> serves;\n";
    out << "    int concurrency = 1;\n";
    out << "};\n\n";

    out << "struct ApiRouteDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string server_name;\n";
    out << "    std::string api_name;\n";
    out << "    std::optional<std::string> method;\n";
    out << "    std::optional<std::string> path;\n";
    out << "    std::optional<std::string> input;\n";
    out << "    std::optional<std::string> output;\n";
    out << "    std::optional<std::string> error;\n";
    out << "};\n\n";

    out << "struct ApiRequestContext\n";
    out << "{\n";
    out << "    std::string server_name;\n";
    out << "    std::string api_name;\n";
    out << "    std::optional<std::string> method;\n";
    out << "    std::optional<std::string> path;\n";
    out << "    statespec::backend::Json body;\n";
    out << "};\n\n";

    out << "struct ApiResponse\n";
    out << "{\n";
    out << "    int status_code = 200;\n";
    out << "    statespec::backend::Json body;\n";
    out << "};\n\n";

    out << "class IApiHandler\n";
    out << "{\n";
    out << "public:\n";
    out << "    virtual ~IApiHandler() = default;\n";
    out << "    virtual ApiResponse handle(const ApiRequestContext& context) = 0;\n";
    out << "};\n\n";

    out << "class IExternalSystemOperatorMetadataApiHandler\n";
    out << "{\n";
    out << "public:\n";
    out << "    virtual ~IExternalSystemOperatorMetadataApiHandler() = default;\n";
    out << "    virtual ApiResponse handle_upsert_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataUpsertRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual ApiResponse handle_get_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataGetRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual ApiResponse handle_disable_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataDisableRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual ApiResponse handle_delete_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataDeleteRequest& request\n";
    out << "    ) = 0;\n";
    out << "};\n\n";

    out << "struct WorkerDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    bool singleton = false;\n";
    out << "    std::optional<std::string> lease;\n";
    out << "    std::optional<std::string> polls;\n";
    out << "    std::optional<std::string> executes;\n";
    out << "    int concurrency = 1;\n";
    out << "};\n\n";

    out << "struct WorkerContext\n";
    out << "{\n";
    out << "    std::string worker_name;\n";
    out << "    bool singleton = false;\n";
    out << "    std::optional<std::string> lease;\n";
    out << "    std::optional<std::string> polls;\n";
    out << "    std::optional<std::string> executes;\n";
    out << "    int concurrency = 1;\n";
    out << "};\n\n";

    out << "class IWorker\n";
    out << "{\n";
    out << "public:\n";
    out << "    virtual ~IWorker() = default;\n";
    out << "    virtual void run(const WorkerContext& context) = 0;\n";
    out << "};\n\n";

    out << "struct PolicyRuleDescriptor\n";
    out << "{\n";
    out << "    std::string action;\n";
    out << "    std::string condition;\n";
    out << "};\n\n";

    out << "struct QuotaDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string expression;\n";
    out << "};\n\n";

    out << "struct PolicyDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> tenant_scoped_by;\n";
    out << "    std::vector<PolicyRuleDescriptor> allows;\n";
    out << "    std::vector<PolicyRuleDescriptor> denies;\n";
    out << "    std::vector<QuotaDescriptor> quotas;\n";
    out << "    std::vector<std::string> audits;\n";
    out << "};\n\n";

    out << "struct ShapeDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> fields;\n";
    out << "};\n\n";

    out << "struct LogDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string level;\n";
    out << "    std::string event_name;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> fields;\n";
    out << "};\n\n";

    out << "struct MetricDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string kind;\n";
    out << "    std::string backend_name;\n";
    out << "    std::string unit;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> labels;\n";
    out << "};\n\n";

    out << "struct GarbageCollectionPolicy\n";
    out << "{\n";
    out << "    std::string after;\n";
    out << "    std::string mode;\n";
    out << "};\n\n";

    out << "struct EntityStateDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    bool terminal = false;\n";
    out << "    std::optional<GarbageCollectionPolicy> garbage_collection;\n";
    out << "};\n\n";

    out << "struct EntityOwnershipDescriptor\n";
    out << "{\n";
    out << "    std::string authority;\n";
    out << "    std::string system_of_record;\n";
    out << "    std::string lifecycle;\n";
    out << "};\n\n";

    out << "struct EntityRelationDescriptor\n";
    out << "{\n";
    out << "    std::string kind;\n";
    out << "    std::string name;\n";
    out << "    std::string target;\n";
    out << "    bool optional = false;\n";
    out << "    std::optional<std::string> relation_kind;\n";
    out << "    std::optional<std::string> on_parent_delete;\n";
    out << "    std::vector<std::string> parent_must_be_in;\n";
    out << "    std::vector<std::string> unique_within_parent;\n";
    out << "};\n\n";

    out << "struct EntityChildDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string target_entity;\n";
    out << "    std::string relation;\n";
    out << "};\n\n";

    out << "struct EntityInvariantDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string expression;\n";
    out << "};\n\n";

    out << "struct EntityDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<std::string> key_fields;\n";
    out << "    std::optional<EntityOwnershipDescriptor> ownership;\n";
    out << "    std::vector<EntityRelationDescriptor> relations;\n";
    out << "    std::vector<EntityChildDescriptor> children;\n";
    out << "    std::vector<EntityInvariantDescriptor> invariants;\n";
    out << "    std::vector<EntityStateDescriptor> states;\n";
    out << "    std::optional<std::string> initial_state;\n";
    out << "    std::vector<std::string> terminal_states;\n";
    out << "};\n\n";

    out << "inline std::vector<FeatureFlagDefinition> feature_flag_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& flag : system.feature_flags)
    {
        out << "        FeatureFlagDefinition{\n";
        out << "            " << cpp_string(flag.name) << ",\n";
        out << "            " << cpp_string(flag.type) << ",\n";
        out << "            " << cpp_string(flag.default_value) << ",\n";
        out << "            " << cpp_string(flag.scope) << ",\n";
        out << "            " << optional_string_expr(flag.owner) << ",\n";
        out << "            " << optional_string_expr(flag.description) << ",\n";
        out << "            " << optional_string_expr(flag.expires) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<ValueDescriptor> value_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& value : system.values)
    {
        out << "        ValueDescriptor{\n";
        out << "            " << cpp_string(value.name) << ",\n";
        out << "            " << cpp_string(value.type) << ",\n";
        out << "            " << optional_string_expr(value.constraint) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<EnumDescriptor> enum_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& enum_decl : system.enums)
    {
        out << "        EnumDescriptor{\n";
        out << "            " << cpp_string(enum_decl.name) << ",\n";
        out << "            {\n";
        for (const auto& member : enum_decl.members)
        {
            out << "                EnumMemberDescriptor{" << cpp_string(member.name) << ", "
                << optional_string_expr(member.value) << "},\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<EventDescriptor> event_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& event : system.events)
    {
        out << "        EventDescriptor{\n";
        out << "            " << cpp_string(event.name) << ",\n";
        out << "            {\n";
        for (const auto& field : event.fields)
        {
            out << "                " << cpp_field_descriptor_expr(field) << ",\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<ExternalSystemDescriptor> external_system_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& external_system : system.external_systems)
    {
        out << "        ExternalSystemDescriptor{\n";
        out << "            " << cpp_string(external_system.name) << ",\n";
        out << "            {\n";
        for (const auto& property : external_system.properties)
        {
            out << "                ExternalSystemPropertyDescriptor{" << cpp_string(property.name)
                << ", " << cpp_string(property.value) << "},\n";
        }
        out << "            },\n";
        if (external_system.metadata.has_value())
        {
            out << "            ExternalSystemMetadataDescriptor{\n";
            out << "                " << cpp_string(external_system.metadata->entity) << ",\n";
            if (external_system.metadata->tenant_field.has_value())
            {
                out << "                " << cpp_string(*external_system.metadata->tenant_field)
                    << ",\n";
            }
            else
            {
                out << "                std::nullopt,\n";
            }
            out << "                " << cpp_string(external_system.metadata->profile_field)
                << ",\n";
            out << "                {";
            for (std::size_t i = 0; i < external_system.metadata->key_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << cpp_string(external_system.metadata->key_fields[i]);
            }
            out << "},\n";
            out << "                {";
            for (std::size_t i = 0; i < external_system.metadata->required_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << cpp_string(external_system.metadata->required_fields[i]);
            }
            out << "},\n";
            out << "                {\n";
            for (const auto& mapping : external_system.metadata->mappings)
            {
                out << "                    ExternalSystemMetadataMappingDescriptor{"
                    << cpp_string(mapping.source) << ", " << cpp_string(mapping.target) << "},\n";
            }
            out << "                },\n";
            out << "            },\n";
        }
        else
        {
            out << "            std::nullopt,\n";
        }
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline ExternalSystemMetadataMappingPlan external_system_metadata_mapping_plan(\n";
    out << "    const ExternalSystemDescriptor& descriptor\n";
    out << ")\n";
    out << "{\n";
    out << "    ExternalSystemMetadataMappingPlan plan;\n";
    out << "    if (!descriptor.metadata.has_value())\n";
    out << "    {\n";
    out << "        return plan;\n";
    out << "    }\n";
    out << "    constexpr std::string_view client_prefix = \"client.\";\n";
    out << "    constexpr std::string_view request_prefix = \"request.\";\n";
    out << "    for (const auto& mapping : descriptor.metadata->mappings)\n";
    out << "    {\n";
    out << "        const auto source_separator = mapping.source.find('.');\n";
    out << "        const auto source_root = source_separator == std::string::npos\n";
    out << "            ? mapping.source\n";
    out << "            : mapping.source.substr(0, source_separator);\n";
    out << "        const auto source_field = source_separator == std::string::npos\n";
    out << "            ? std::string{}\n";
    out << "            : mapping.source.substr(source_separator + 1);\n";
    out << "        if (mapping.target.rfind(client_prefix, 0) == 0)\n";
    out << "        {\n";
    out << "            auto assignment = ExternalSystemMetadataMappingAssignment{\n";
    out << "                mapping.source, source_root, source_field, \"client\",\n";
    out << "                mapping.target.substr(client_prefix.size()),\n";
    out << "            };\n";
    out << "            plan.all_mappings.push_back(assignment);\n";
    out << "            plan.client_mappings.push_back(std::move(assignment));\n";
    out << "        }\n";
    out << "        else if (mapping.target.rfind(request_prefix, 0) == 0)\n";
    out << "        {\n";
    out << "            auto assignment = ExternalSystemMetadataMappingAssignment{\n";
    out << "                mapping.source, source_root, source_field, \"request\",\n";
    out << "                mapping.target.substr(request_prefix.size()),\n";
    out << "            };\n";
    out << "            plan.all_mappings.push_back(assignment);\n";
    out << "            plan.request_mappings.push_back(std::move(assignment));\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return plan;\n";
    out << "}\n\n";

    out << "inline std::optional<statespec::backend::ExternalSystemMetadataLookup> "
           "external_system_metadata_lookup(\n";
    out << "    const ExternalSystemDescriptor& descriptor,\n";
    out << "    std::vector<statespec::backend::ExternalSystemMetadataKeyValue> key_values\n";
    out << ")\n";
    out << "{\n";
    out << "    if (!descriptor.metadata.has_value())\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    out << "    const auto& metadata = *descriptor.metadata;\n";
    out << "    return statespec::backend::ExternalSystemMetadataLookup{\n";
    out << "        descriptor.name,\n";
    out << "        metadata.entity,\n";
    out << "        metadata.tenant_field,\n";
    out << "        metadata.profile_field,\n";
    out << "        metadata.key_fields,\n";
    out << "        std::move(key_values),\n";
    out << "        metadata.required_fields,\n";
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::optional<statespec::backend::ExternalSystemMetadataLookup> "
           "external_system_metadata_lookup(\n";
    out << "    std::string_view external_system,\n";
    out << "    std::vector<statespec::backend::ExternalSystemMetadataKeyValue> key_values\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& descriptor : external_system_descriptors())\n";
    out << "    {\n";
    out << "        if (descriptor.name == external_system)\n";
    out << "        {\n";
    out << "            return external_system_metadata_lookup(descriptor, "
           "std::move(key_values));\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return std::nullopt;\n";
    out << "}\n\n";

    out << "inline std::optional<statespec::backend::ExternalSystemMetadataResolution> "
           "resolve_external_system_metadataTx(\n";
    out << "    statespec::backend::IExternalSystemMetadataResolver& resolver,\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    const ExternalSystemDescriptor& descriptor,\n";
    out << "    std::vector<statespec::backend::ExternalSystemMetadataKeyValue> key_values\n";
    out << ")\n";
    out << "{\n";
    out << "    auto lookup = external_system_metadata_lookup(descriptor, "
           "std::move(key_values));\n";
    out << "    if (!lookup.has_value())\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    out << "    if (!lookup->key_complete())\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    out << "    return resolver.resolve_metadataTx(tx, *lookup);\n";
    out << "}\n\n";

    out << "inline std::optional<statespec::backend::ExternalSystemMetadataResolution> "
           "resolve_external_system_metadataTx(\n";
    out << "    statespec::backend::IExternalSystemMetadataResolver& resolver,\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    std::string_view external_system,\n";
    out << "    std::vector<statespec::backend::ExternalSystemMetadataKeyValue> key_values\n";
    out << ")\n";
    out << "{\n";
    out << "    auto lookup = external_system_metadata_lookup(external_system, "
           "std::move(key_values));\n";
    out << "    if (!lookup.has_value())\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    out << "    if (!lookup->key_complete())\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    out << "    return resolver.resolve_metadataTx(tx, *lookup);\n";
    out << "}\n\n";

    out << "inline std::vector<ApiDescriptor> api_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& api : system.apis)
    {
        out << "        ApiDescriptor{\n";
        out << "            " << cpp_string(api.name) << ",\n";
        out << "            " << optional_string_expr(api.method) << ",\n";
        out << "            " << optional_string_expr(api.path) << ",\n";
        out << "            " << optional_string_expr(api.input) << ",\n";
        out << "            " << optional_string_expr(api.output) << ",\n";
        out << "            " << optional_string_expr(api.error) << ",\n";
        out << "            " << optional_string_expr(api.starts_workflow) << ",\n";
        out << "            " << optional_string_expr(api.enqueues) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<ApiServerDescriptor> api_server_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "        ApiServerDescriptor{\n";
        out << "            " << cpp_string(api_server.name) << ",\n";
        out << "            {";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_string(api_server.serves[i]);
        }
        out << "},\n";
        out << "            " << api_server.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<ApiRouteDescriptor> api_route_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api_name : api_server.serves)
        {
            const auto* api = find_api(system, api_name);
            out << "        ApiRouteDescriptor{\n";
            out << "            " << cpp_string(api_server.name + "." + api_name) << ",\n";
            out << "            " << cpp_string(api_server.name) << ",\n";
            out << "            " << cpp_string(api_name) << ",\n";
            out << "            " << optional_string_expr(optional_api_field(api, &IrApi::method))
                << ",\n";
            out << "            " << optional_string_expr(optional_api_field(api, &IrApi::path))
                << ",\n";
            out << "            " << optional_string_expr(optional_api_field(api, &IrApi::input))
                << ",\n";
            out << "            " << optional_string_expr(optional_api_field(api, &IrApi::output))
                << ",\n";
            out << "            " << optional_string_expr(optional_api_field(api, &IrApi::error))
                << ",\n";
            out << "        },\n";
        }
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<WorkerDescriptor> worker_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& worker : system.workers)
    {
        out << "        WorkerDescriptor{\n";
        out << "            " << cpp_string(worker.name) << ",\n";
        out << "            " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
        out << "            " << optional_string_expr(worker.lease) << ",\n";
        out << "            " << optional_string_expr(worker.polls) << ",\n";
        out << "            " << optional_string_expr(worker.executes) << ",\n";
        out << "            " << worker.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<WorkerContext> worker_contexts()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& worker : system.workers)
    {
        out << "        WorkerContext{\n";
        out << "            " << cpp_string(worker.name) << ",\n";
        out << "            " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
        out << "            " << optional_string_expr(worker.lease) << ",\n";
        out << "            " << optional_string_expr(worker.polls) << ",\n";
        out << "            " << optional_string_expr(worker.executes) << ",\n";
        out << "            " << worker.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<PolicyDescriptor> policy_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& policy : system.policies)
    {
        out << "        PolicyDescriptor{\n";
        out << "            " << cpp_string(policy.name) << ",\n";
        out << "            " << optional_string_expr(policy.tenant_scoped_by) << ",\n";
        out << "            {\n";
        for (const auto& allow : policy.allows)
        {
            out << "                PolicyRuleDescriptor{" << cpp_string(allow.action) << ", "
                << cpp_string(allow.condition) << "},\n";
        }
        out << "            },\n";
        out << "            {\n";
        for (const auto& deny : policy.denies)
        {
            out << "                PolicyRuleDescriptor{" << cpp_string(deny.action) << ", "
                << cpp_string(deny.condition) << "},\n";
        }
        out << "            },\n";
        out << "            {\n";
        for (const auto& quota : policy.quotas)
        {
            out << "                QuotaDescriptor{" << cpp_string(quota.name) << ", "
                << cpp_string(quota.expression) << "},\n";
        }
        out << "            },\n";
        out << "            {";
        for (std::size_t i = 0; i < policy.audits.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_string(policy.audits[i]);
        }
        out << "},\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<ShapeDescriptor> shape_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& shape : system.shapes)
    {
        out << "        ShapeDescriptor{\n";
        out << "            " << cpp_string(shape.name) << ",\n";
        out << "            {\n";
        for (const auto& field : shape.fields)
        {
            out << "                " << cpp_field_descriptor_expr(field) << ",\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<LogDefinition> log_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& log : system.logs)
    {
        out << "        LogDefinition{\n";
        out << "            " << cpp_string(log.name) << ",\n";
        out << "            " << cpp_string(log.level) << ",\n";
        out << "            " << cpp_string(log.event_name) << ",\n";
        out << "            {\n";
        for (const auto& field : log.fields)
        {
            out << "                " << cpp_field_descriptor_expr(field) << ",\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<MetricDefinition> metric_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& metric : system.metrics)
    {
        out << "        MetricDefinition{\n";
        out << "            " << cpp_string(metric.name) << ",\n";
        out << "            " << cpp_string(metric.kind) << ",\n";
        out << "            " << cpp_string(metric.backend_name) << ",\n";
        out << "            " << cpp_string(metric.unit) << ",\n";
        out << "            {\n";
        for (const auto& label : metric.labels)
        {
            out << "                " << cpp_field_descriptor_expr(label) << ",\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

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

    out << "inline std::vector<statespec::backend::QueueDefinition> queue_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& queue : system.queues)
    {
        out << "        statespec::backend::QueueDefinition{\n";
        out << "            " << cpp_string(queue.name) << ",\n";
        out << "            " << cpp_string(queue.channel.value_or("default")) << ",\n";
        out << "            std::chrono::seconds{"
            << parse_duration_seconds(queue.visibility_timeout) << "},\n";
        out << "            " << queue.max_attempts.value_or(1) << ",\n";
        out << "            " << optional_string_expr(queue.dead_letter) << ",\n";
        out << "            \"{}\",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<LeaseDefinition> lease_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& lease : system.leases)
    {
        out << "        LeaseDefinition{\n";
        out << "            " << cpp_string(lease.name) << ",\n";
        out << "            " << optional_string_expr(lease.resource) << ",\n";
        out << "            std::chrono::seconds{" << parse_duration_seconds(lease.ttl) << "},\n";
        if (lease.renew_every.has_value())
        {
            out << "            std::optional<std::chrono::seconds>{std::chrono::seconds{"
                << parse_duration_seconds(lease.renew_every) << "}},\n";
        }
        else
        {
            out << "            std::nullopt,\n";
        }
        out << "            " << optional_string_expr(lease.holder) << ",\n";
        out << "            " << (lease.fencing_token.value_or(false) ? "true" : "false") << ",\n";
        if (lease.max_ttl.has_value())
        {
            out << "            std::optional<std::chrono::seconds>{std::chrono::seconds{"
                << parse_duration_seconds(lease.max_ttl) << "}},\n";
        }
        else
        {
            out << "            std::nullopt,\n";
        }
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<statespec::backend::WorkflowDefinition> workflow_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& workflow : system.workflows)
    {
        out << "        statespec::backend::WorkflowDefinition{\n";
        out << "            " << cpp_string(workflow.name) << ",\n";
        out << "            " << workflow.version.value_or(1) << ",\n";
        out << "            " << cpp_string(workflow.start_step.value_or("")) << ",\n";
        out << "            std::chrono::seconds{"
            << parse_duration_seconds(workflow.expected_execution_time) << "},\n";
        out << "            " << (workflow.singleton.value_or(false) ? "true" : "false") << ",\n";
        out << "            {\n";
        for (const auto& step : workflow.steps)
        {
            out << "                statespec::backend::WorkflowStepDefinition{"
                << cpp_string(step.name) << ", std::chrono::seconds{"
                << parse_duration_seconds(step.expected_execution_time) << "}, "
                << step.max_retries.value_or(0) << "},\n";
        }
        out << "            },\n";
        out << "            \"{}\",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline statespec::backend::LogLevel log_level_from_string(std::string_view level)\n";
    out << "{\n";
    out << "    if (level == \"debug\") { return statespec::backend::LogLevel::Debug; }\n";
    out << "    if (level == \"warn\") { return statespec::backend::LogLevel::Warn; }\n";
    out << "    if (level == \"error\") { return statespec::backend::LogLevel::Error; }\n";
    out << "    return statespec::backend::LogLevel::Info;\n";
    out << "}\n\n";

    out << "inline statespec::backend::MetricKind metric_kind_from_string(std::string_view kind)\n";
    out << "{\n";
    out << "    if (kind == \"gauge\") { return statespec::backend::MetricKind::Gauge; }\n";
    out << "    if (kind == \"histogram\") { return statespec::backend::MetricKind::Histogram; }\n";
    out << "    return statespec::backend::MetricKind::Counter;\n";
    out << "}\n\n";

    out << "inline statespec::backend::FeatureFlagType feature_flag_type_from_string(\n";
    out << "    std::string_view type\n";
    out << ")\n";
    out << "{\n";
    out << "    if (type == \"string\") { return statespec::backend::FeatureFlagType::String; }\n";
    out << "    if (type == \"int\") { return statespec::backend::FeatureFlagType::Integer; }\n";
    out << "    if (type == \"decimal\") { return statespec::backend::FeatureFlagType::Decimal; "
           "}\n";
    out << "    return statespec::backend::FeatureFlagType::Bool;\n";
    out << "}\n\n";

    out << "inline statespec::backend::FeatureFlagScopeKind feature_flag_scope_from_string(\n";
    out << "    std::string_view scope\n";
    out << ")\n";
    out << "{\n";
    out << "    if (scope == \"system\") { return "
           "statespec::backend::FeatureFlagScopeKind::System; }\n";
    out << "    if (scope == \"user\") { return statespec::backend::FeatureFlagScopeKind::User; "
           "}\n";
    out << "    if (scope.rfind(\"entity \", 0) == 0) { return "
           "statespec::backend::FeatureFlagScopeKind::Entity; }\n";
    out << "    return statespec::backend::FeatureFlagScopeKind::Tenant;\n";
    out << "}\n\n";

    out << "inline statespec::backend::FeatureFlagValue feature_flag_value_from_descriptor(\n";
    out << "    const FeatureFlagDefinition& definition\n";
    out << ")\n";
    out << "{\n";
    out << "    if (definition.type == \"string\")\n";
    out << "    {\n";
    out << "        return "
           "statespec::backend::FeatureFlagValue::string_value(definition.default_value);\n";
    out << "    }\n";
    out << "    if (definition.type == \"int\")\n";
    out << "    {\n";
    out << "        return "
           "statespec::backend::FeatureFlagValue::integer_value(std::stoll(definition.default_"
           "value));\n";
    out << "    }\n";
    out << "    if (definition.type == \"decimal\")\n";
    out << "    {\n";
    out << "        return "
           "statespec::backend::FeatureFlagValue::decimal_value(std::stod(definition.default_value)"
           ");\n";
    out << "    }\n";
    out << "    return statespec::backend::FeatureFlagValue::bool_value(definition.default_value "
           "== \"true\");\n";
    out << "}\n\n";

    out << "inline void ensure_system_collections(statespec::backend::IBackend& backend)\n";
    out << "{\n";
    out << "    backend.ensure_collections(collection_descriptors());\n";
    out << "}\n\n";

    out << "inline void register_feature_flag_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IFeatureFlagStore& store\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : feature_flag_definitions())\n";
    out << "    {\n";
    out << "        store.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::FeatureFlagDefinition{\n";
    out << "                definition.name,\n";
    out << "                feature_flag_type_from_string(definition.type),\n";
    out << "                feature_flag_value_from_descriptor(definition),\n";
    out << "                feature_flag_scope_from_string(definition.scope),\n";
    out << "                definition.owner,\n";
    out << "                definition.description,\n";
    out << "                definition.expires,\n";
    out << "            }\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void create_queue_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IQueueStore& store\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : queue_definitions())\n";
    out << "    {\n";
    out << "        store.createTx(tx, statespec::backend::CreateQueueRequest{definition});\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline statespec::backend::LeaseDefinition lease_definition_from_descriptor(\n";
    out << "    const LeaseDefinition& definition\n";
    out << ")\n";
    out << "{\n";
    out << "    return statespec::backend::LeaseDefinition{\n";
    out << "        statespec::backend::LeaseDefinitionId{definition.name, 1},\n";
    out << "        definition.resource.value_or(definition.name),\n";
    out << "        definition.ttl,\n";
    out << "        definition.renew_every.value_or(definition.ttl),\n";
    out << "        definition.max_ttl,\n";
    out << "        definition.fencing_token,\n";
    out << "    };\n";
    out << "}\n\n";

    out << "inline void register_lease_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::ILeaseStore& store\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : lease_definitions())\n";
    out << "    {\n";
    out << "        store.register_definitionTx(tx, "
           "lease_definition_from_descriptor(definition));\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void register_log_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::ILogSink& sink\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : log_definitions())\n";
    out << "    {\n";
    out << "        sink.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::LogDefinition{\n";
    out << "                definition.name,\n";
    out << "                log_level_from_string(definition.level),\n";
    out << "                definition.event_name,\n";
    out << "                definition.fields,\n";
    out << "            }\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void register_metric_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IMetricSink& sink\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : metric_definitions())\n";
    out << "    {\n";
    out << "        sink.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::MetricDefinition{\n";
    out << "                definition.name,\n";
    out << "                metric_kind_from_string(definition.kind),\n";
    out << "                definition.backend_name,\n";
    out << "                definition.unit,\n";
    out << "                definition.labels,\n";
    out << "            }\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void register_observability_catalogTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::ILogSink& log_sink,\n";
    out << "    statespec::backend::IMetricSink& metric_sink\n";
    out << ")\n";
    out << "{\n";
    out << "    register_log_definitionsTx(tx, log_sink);\n";
    out << "    register_metric_definitionsTx(tx, metric_sink);\n";
    out << "}\n\n";

    out << "inline void register_workflow_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IWorkflowStore& workflow_store\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : workflow_definitions())\n";
    out << "    {\n";
    out << "        workflow_store.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::RegisterWorkflowDefinitionRequest{definition}\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void register_runtime_catalogTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IFeatureFlagStore& feature_flag_store,\n";
    out << "    statespec::backend::IQueueStore& queue_store,\n";
    out << "    statespec::backend::ILeaseStore& lease_store,\n";
    out << "    statespec::backend::IWorkflowStore& workflow_store,\n";
    out << "    statespec::backend::ILogSink& log_sink,\n";
    out << "    statespec::backend::IMetricSink& metric_sink\n";
    out << ")\n";
    out << "{\n";
    out << "    register_feature_flag_definitionsTx(tx, feature_flag_store);\n";
    out << "    create_queue_definitionsTx(tx, queue_store);\n";
    out << "    register_lease_definitionsTx(tx, lease_store);\n";
    out << "    register_workflow_definitionsTx(tx, workflow_store);\n";
    out << "    register_observability_catalogTx(tx, log_sink, metric_sink);\n";
    out << "}\n\n";

    out << "} // namespace statespec_generated\n";
    return out.str();
}

std::string generate_api_artifacts_header()
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"common/system_descriptors.hpp\"\n\n";
    out << "namespace statespec_generated::api\n";
    out << "{\n\n";
    out << "using ApiDescriptor = ::statespec_generated::ApiDescriptor;\n";
    out << "using ApiRequestContext = ::statespec_generated::ApiRequestContext;\n";
    out << "using ApiResponse = ::statespec_generated::ApiResponse;\n";
    out << "using ApiRouteDescriptor = ::statespec_generated::ApiRouteDescriptor;\n";
    out << "using ApiServerDescriptor = ::statespec_generated::ApiServerDescriptor;\n";
    out << "using IApiHandler = ::statespec_generated::IApiHandler;\n";
    out << "using IExternalSystemOperatorMetadataApiHandler = "
           "::statespec_generated::IExternalSystemOperatorMetadataApiHandler;\n\n";
    out << "inline std::vector<ApiDescriptor> api_descriptors()\n";
    out << "{\n";
    out << "    return ::statespec_generated::api_descriptors();\n";
    out << "}\n\n";
    out << "inline std::vector<ApiServerDescriptor> api_server_descriptors()\n";
    out << "{\n";
    out << "    return ::statespec_generated::api_server_descriptors();\n";
    out << "}\n\n";
    out << "inline std::vector<ApiRouteDescriptor> api_route_descriptors()\n";
    out << "{\n";
    out << "    return ::statespec_generated::api_route_descriptors();\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::api\n";
    return out.str();
}

std::string generate_worker_artifacts_header()
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"common/system_descriptors.hpp\"\n\n";
    out << "namespace statespec_generated::worker\n";
    out << "{\n\n";
    out << "using IWorker = ::statespec_generated::IWorker;\n";
    out << "using LeaseDefinition = ::statespec_generated::LeaseDefinition;\n";
    out << "using WorkerContext = ::statespec_generated::WorkerContext;\n";
    out << "using WorkerDescriptor = ::statespec_generated::WorkerDescriptor;\n\n";
    out << "inline std::vector<WorkerDescriptor> worker_descriptors()\n";
    out << "{\n";
    out << "    return ::statespec_generated::worker_descriptors();\n";
    out << "}\n\n";
    out << "inline std::vector<WorkerContext> worker_contexts()\n";
    out << "{\n";
    out << "    return ::statespec_generated::worker_contexts();\n";
    out << "}\n\n";
    out << "inline std::vector<statespec::backend::QueueDefinition> queue_definitions()\n";
    out << "{\n";
    out << "    return ::statespec_generated::queue_definitions();\n";
    out << "}\n\n";
    out << "inline std::vector<LeaseDefinition> lease_definitions()\n";
    out << "{\n";
    out << "    return ::statespec_generated::lease_definitions();\n";
    out << "}\n\n";
    out << "inline std::vector<statespec::backend::WorkflowDefinition> workflow_definitions()\n";
    out << "{\n";
    out << "    return ::statespec_generated::workflow_definitions();\n";
    out << "}\n\n";
    out << "inline void create_queue_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IQueueStore& queue_store\n";
    out << ")\n";
    out << "{\n";
    out << "    ::statespec_generated::create_queue_definitionsTx(tx, queue_store);\n";
    out << "}\n\n";
    out << "inline void register_lease_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::ILeaseStore& lease_store\n";
    out << ")\n";
    out << "{\n";
    out << "    ::statespec_generated::register_lease_definitionsTx(tx, lease_store);\n";
    out << "}\n\n";
    out << "inline void register_workflow_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IWorkflowStore& workflow_store\n";
    out << ")\n";
    out << "{\n";
    out << "    ::statespec_generated::register_workflow_definitionsTx(tx, workflow_store);\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::worker\n";
    return out.str();
}

} // namespace

GenerationResult generate_cpp_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/cpp"};

    add_template_file(
        result, options.output_dir, template_root / "json.hpp", "json.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "backend.hpp", "backend.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "external_system.hpp", "external_system.hpp",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "feature_flag.hpp", "feature_flag.hpp",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "lease.hpp", "lease.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "log.hpp", "log.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "metric.hpp", "metric.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "queue.hpp", "queue.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "workflow.hpp", "workflow.hpp", diagnostics
    );

    if (!diagnostics.has_errors())
    {
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "common/system_descriptors.hpp").string(),
                generate_system_descriptors_header(system),
                GeneratedArtifactTier::Common,
                "common/system_descriptors.hpp",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "Makefile").string(),
                generate_makefile(options.tier),
                GeneratedArtifactTier::Common,
                "common/Makefile",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "api_artifacts.hpp").string(),
                generate_api_artifacts_header(),
                GeneratedArtifactTier::Api,
                "api/api_artifacts.hpp",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "worker_artifacts.hpp").string(),
                generate_worker_artifacts_header(),
                GeneratedArtifactTier::Worker,
                "worker/worker_artifacts.hpp",
            }
        );
    }

    return result;
}

} // namespace statespec
