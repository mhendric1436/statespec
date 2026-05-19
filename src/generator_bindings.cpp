#include "statespec/generator_bindings.hpp"

#include <cctype>
#include <stdexcept>

namespace statespec
{

namespace
{

std::string trim_copy(const std::string& value)
{
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0)
    {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return value.substr(begin, end - begin);
}

bool starts_with(
    const std::string& value,
    const std::string& prefix
)
{
    return value.rfind(prefix, 0) == 0;
}

bool ends_with(
    const std::string& value,
    const std::string& suffix
)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool validate_binding_generation_request(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    bool valid = true;

    if (!spec.system.has_value())
    {
        diagnostics.error(
            SourceRange{}, "SSPEC5101", "binding generation requires a system declaration"
        );
        valid = false;
    }

    if (options.output_dir.empty())
    {
        diagnostics.error(
            SourceRange{}, "SSPEC5102", "binding generation requires an output directory"
        );
        valid = false;
    }

    return valid;
}

bool generated_file_selected_for_tier(
    const GeneratedFile& file,
    BindingGenerationTier tier
)
{
    switch (tier)
    {
    case BindingGenerationTier::All:
        return true;
    case BindingGenerationTier::Common:
        return file.tier == GeneratedArtifactTier::Common;
    case BindingGenerationTier::Api:
        return file.tier == GeneratedArtifactTier::Common ||
               file.tier == GeneratedArtifactTier::Api;
    case BindingGenerationTier::Worker:
        return file.tier == GeneratedArtifactTier::Common ||
               file.tier == GeneratedArtifactTier::Worker;
    }
    return true;
}

GenerationResult filter_generation_result_by_tier(
    GenerationResult result,
    BindingGenerationTier tier
)
{
    if (tier == BindingGenerationTier::All)
    {
        return result;
    }

    GenerationResult filtered;
    for (const auto& file : result.files)
    {
        if (generated_file_selected_for_tier(file, tier))
        {
            filtered.files.push_back(file);
        }
    }
    return filtered;
}

} // namespace

BindingGenerationTier parse_binding_generation_tier(const std::string& value)
{
    const auto normalized = trim_copy(value);
    if (normalized == "all")
    {
        return BindingGenerationTier::All;
    }
    if (normalized == "common")
    {
        return BindingGenerationTier::Common;
    }
    if (normalized == "api")
    {
        return BindingGenerationTier::Api;
    }
    if (normalized == "worker")
    {
        return BindingGenerationTier::Worker;
    }
    throw std::invalid_argument(
        "unsupported binding generation tier '" + value +
        "'; supported tiers: " + supported_binding_generation_tiers_text()
    );
}

std::string binding_generation_tier_name(BindingGenerationTier tier)
{
    switch (tier)
    {
    case BindingGenerationTier::All:
        return "all";
    case BindingGenerationTier::Common:
        return "common";
    case BindingGenerationTier::Api:
        return "api";
    case BindingGenerationTier::Worker:
        return "worker";
    }
    return "all";
}

std::string supported_binding_generation_tiers_text()
{
    return "all|common|api|worker";
}

FieldDescriptorType classify_field_descriptor_type(const std::string& type_name)
{
    const auto normalized = trim_copy(type_name);
    if (normalized == "string")
    {
        return FieldDescriptorType::String;
    }
    if (normalized == "bool")
    {
        return FieldDescriptorType::Bool;
    }
    if (normalized == "int")
    {
        return FieldDescriptorType::Int;
    }
    if (normalized == "int32")
    {
        return FieldDescriptorType::Int32;
    }
    if (normalized == "int64")
    {
        return FieldDescriptorType::Int64;
    }
    if (normalized == "long")
    {
        return FieldDescriptorType::Long;
    }
    if (normalized == "double")
    {
        return FieldDescriptorType::Double;
    }
    if (normalized == "decimal")
    {
        return FieldDescriptorType::Decimal;
    }
    if (normalized == "json")
    {
        return FieldDescriptorType::Json;
    }
    if (normalized == "timestamp")
    {
        return FieldDescriptorType::Timestamp;
    }
    if (normalized == "duration")
    {
        return FieldDescriptorType::Duration;
    }
    if (normalized == "uuid")
    {
        return FieldDescriptorType::Uuid;
    }
    if (ends_with(normalized, "?") || starts_with(normalized, "optional<"))
    {
        return FieldDescriptorType::Optional;
    }
    if (starts_with(normalized, "list<") || ends_with(normalized, "[]"))
    {
        return FieldDescriptorType::List;
    }
    if (starts_with(normalized, "set<"))
    {
        return FieldDescriptorType::Set;
    }
    if (starts_with(normalized, "map<"))
    {
        return FieldDescriptorType::Map;
    }
    if (starts_with(normalized, "ref<"))
    {
        return FieldDescriptorType::Reference;
    }
    return FieldDescriptorType::Named;
}

std::string field_descriptor_type_name(FieldDescriptorType type)
{
    switch (type)
    {
    case FieldDescriptorType::String:
        return "string";
    case FieldDescriptorType::Bool:
        return "bool";
    case FieldDescriptorType::Int:
        return "int";
    case FieldDescriptorType::Int32:
        return "int32";
    case FieldDescriptorType::Int64:
        return "int64";
    case FieldDescriptorType::Long:
        return "long";
    case FieldDescriptorType::Double:
        return "double";
    case FieldDescriptorType::Decimal:
        return "decimal";
    case FieldDescriptorType::Json:
        return "json";
    case FieldDescriptorType::Timestamp:
        return "timestamp";
    case FieldDescriptorType::Duration:
        return "duration";
    case FieldDescriptorType::Uuid:
        return "uuid";
    case FieldDescriptorType::Named:
        return "named";
    case FieldDescriptorType::List:
        return "list";
    case FieldDescriptorType::Set:
        return "set";
    case FieldDescriptorType::Map:
        return "map";
    case FieldDescriptorType::Optional:
        return "optional";
    case FieldDescriptorType::Reference:
        return "reference";
    }
    return "named";
}

GenerationResult generate_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    if (!validate_binding_generation_request(spec, options, diagnostics))
    {
        return GenerationResult{};
    }

    const auto system = lower_to_ir(spec);

    switch (options.language)
    {
    case BindingLanguage::Cpp:
        return filter_generation_result_by_tier(
            generate_cpp_bindings(system, options, diagnostics), options.tier
        );
    case BindingLanguage::Go:
        return filter_generation_result_by_tier(
            generate_go_bindings(system, options, diagnostics), options.tier
        );
    case BindingLanguage::Java:
        return filter_generation_result_by_tier(
            generate_java_bindings(system, options, diagnostics), options.tier
        );
    case BindingLanguage::Rust:
        return filter_generation_result_by_tier(
            generate_rust_bindings(system, options, diagnostics), options.tier
        );
    }

    diagnostics.error(SourceRange{}, "SSPEC5104", "unsupported binding generator language");
    return GenerationResult{};
}

} // namespace statespec
