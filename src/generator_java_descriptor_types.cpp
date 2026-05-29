#include "generator_java_artifacts.hpp"

namespace statespec
{

std::vector<std::pair<
    std::string,
    std::string>>
generate_java_descriptor_type_files()
{
    const std::string package_line = "package com.statespec.generated.descriptors.types;\n\n";
    auto file = [&](std::string filename, std::string imports, std::string body)
    {
        return std::pair<std::string, std::string>{
            std::move(filename), package_line + std::move(imports) + std::move(body)
        };
    };
    std::vector<std::pair<std::string, std::string>> files;
    files.push_back(file(
        "EventEnvelope.java", "import com.statespec.backend.Json;\nimport java.util.Map;\n\n",
        "public record EventEnvelope(String name, Map<String, Json> fields) {}\n"
    ));
    files.push_back(file(
        "ValueDescriptor.java", "import java.util.Optional;\n\n",
        "public record ValueDescriptor(String name, String type, Optional<String> constraint) {}\n"
    ));
    files.push_back(file(
        "EnumMemberDescriptor.java", "import java.util.Optional;\n\n",
        "public record EnumMemberDescriptor(String name, Optional<String> value) {}\n"
    ));
    files.push_back(file(
        "EnumDescriptor.java", "import java.util.List;\n\n",
        "public record EnumDescriptor(String name, List<EnumMemberDescriptor> members) {}\n"
    ));
    files.push_back(file(
        "EventDescriptor.java",
        "import com.statespec.backend.Backend.FieldDescriptor;\n"
        "import java.util.List;\n\n",
        "public record EventDescriptor(String name, List<FieldDescriptor> fields) {}\n"
    ));
    files.push_back(file(
        "ExternalSystemPropertyDescriptor.java", "",
        "public record ExternalSystemPropertyDescriptor(String name, String value) {}\n"
    ));
    files.push_back(file(
        "ExternalSystemMetadataMappingDescriptor.java", "",
        "public record ExternalSystemMetadataMappingDescriptor(String source, String target) {}\n"
    ));
    files.push_back(file(
        "ExternalSystemMetadataDescriptor.java",
        "import java.util.List;\nimport java.util.Optional;\n\n",
        "public record ExternalSystemMetadataDescriptor(\n"
        "    String entity,\n"
        "    Optional<String> tenantField,\n"
        "    String profileField,\n"
        "    List<String> keyFields,\n"
        "    List<String> requiredFields,\n"
        "    List<ExternalSystemMetadataMappingDescriptor> mappings\n"
        ") {}\n"
    ));
    files.push_back(file(
        "ExternalSystemDescriptor.java", "import java.util.List;\nimport java.util.Optional;\n\n",
        "public record ExternalSystemDescriptor(\n"
        "    String name,\n"
        "    List<ExternalSystemPropertyDescriptor> properties,\n"
        "    Optional<ExternalSystemMetadataDescriptor> metadata\n"
        ") {}\n"
    ));
    files.push_back(file(
        "ApiDescriptor.java", "import java.util.Optional;\n\n",
        "public record ApiDescriptor(\n"
        "    String name,\n"
        "    Optional<String> method,\n"
        "    Optional<String> path,\n"
        "    Optional<String> input,\n"
        "    Optional<String> output,\n"
        "    Optional<String> error,\n"
        "    Optional<String> startsWorkflow,\n"
        "    Optional<String> enqueues\n"
        ") {}\n"
    ));
    files.push_back(file(
        "ApiServerDescriptor.java", "import java.util.List;\n\n",
        "public record ApiServerDescriptor(String name, List<String> serves, int concurrency) {}\n"
    ));
    files.push_back(file(
        "ApiRouteDescriptor.java", "import java.util.Optional;\n\n",
        "public record ApiRouteDescriptor(\n"
        "    String name,\n"
        "    String serverName,\n"
        "    String apiName,\n"
        "    Optional<String> method,\n"
        "    Optional<String> path,\n"
        "    Optional<String> input,\n"
        "    Optional<String> output,\n"
        "    Optional<String> error\n"
        ") {}\n"
    ));
    files.push_back(file(
        "WorkerDescriptor.java", "import java.util.Optional;\n\n",
        "public record WorkerDescriptor(\n"
        "    String name,\n"
        "    boolean singleton,\n"
        "    Optional<String> lease,\n"
        "    Optional<String> polls,\n"
        "    Optional<String> executes,\n"
        "    int concurrency\n"
        ") {}\n"
    ));
    files.push_back(file(
        "WorkerContext.java", "import java.util.Optional;\n\n",
        "public record WorkerContext(\n"
        "    String workerName,\n"
        "    boolean singleton,\n"
        "    Optional<String> lease,\n"
        "    Optional<String> polls,\n"
        "    Optional<String> executes,\n"
        "    int concurrency\n"
        ") {}\n"
    ));
    files.push_back(file(
        "PolicyRuleDescriptor.java", "",
        "public record PolicyRuleDescriptor(String action, String condition) {}\n"
    ));
    files.push_back(file(
        "QuotaDescriptor.java", "",
        "public record QuotaDescriptor(String name, String expression) {}\n"
    ));
    files.push_back(file(
        "PolicyDescriptor.java", "import java.util.List;\nimport java.util.Optional;\n\n",
        "public record PolicyDescriptor(\n"
        "    String name,\n"
        "    Optional<String> tenantScopedBy,\n"
        "    List<PolicyRuleDescriptor> allows,\n"
        "    List<PolicyRuleDescriptor> denies,\n"
        "    List<QuotaDescriptor> quotas,\n"
        "    List<String> audits\n"
        ") {}\n"
    ));
    files.push_back(file(
        "ShapeDescriptor.java",
        "import com.statespec.backend.Backend.FieldDescriptor;\n"
        "import java.util.List;\n\n",
        "public record ShapeDescriptor(String name, List<FieldDescriptor> fields) {}\n"
    ));
    files.push_back(file(
        "GarbageCollectionPolicy.java", "",
        "public record GarbageCollectionPolicy(String after, String mode) {}\n"
    ));
    files.push_back(file(
        "EntityStateDescriptor.java", "import java.util.Optional;\n\n",
        "public record EntityStateDescriptor(\n"
        "    String name,\n"
        "    boolean terminal,\n"
        "    Optional<GarbageCollectionPolicy> garbageCollection\n"
        ") {}\n"
    ));
    files.push_back(file(
        "EntityOwnershipDescriptor.java", "",
        "public record EntityOwnershipDescriptor(\n"
        "    String authority,\n"
        "    String systemOfRecord,\n"
        "    String lifecycle\n"
        ") {}\n"
    ));
    files.push_back(file(
        "EntityRelationDescriptor.java", "import java.util.List;\nimport java.util.Optional;\n\n",
        "public record EntityRelationDescriptor(\n"
        "    String kind,\n"
        "    String name,\n"
        "    String target,\n"
        "    boolean optional,\n"
        "    Optional<String> relationKind,\n"
        "    Optional<String> onParentDelete,\n"
        "    List<String> parentMustBeIn,\n"
        "    List<String> uniqueWithinParent\n"
        ") {}\n"
    ));
    files.push_back(file(
        "EntityChildDescriptor.java", "",
        "public record EntityChildDescriptor(String name, String targetEntity, String relation) "
        "{}\n"
    ));
    files.push_back(file(
        "EntityInvariantDescriptor.java", "",
        "public record EntityInvariantDescriptor(String name, String expression) {}\n"
    ));
    files.push_back(file(
        "EntityDescriptor.java", "import java.util.List;\nimport java.util.Optional;\n\n",
        "public record EntityDescriptor(\n"
        "    String name,\n"
        "    List<String> keyFields,\n"
        "    Optional<EntityOwnershipDescriptor> ownership,\n"
        "    List<EntityRelationDescriptor> relations,\n"
        "    List<EntityChildDescriptor> children,\n"
        "    List<EntityInvariantDescriptor> invariants,\n"
        "    List<EntityStateDescriptor> states,\n"
        "    Optional<String> initialState,\n"
        "    List<String> terminalStates\n"
        ") {}\n"
    ));
    return files;
}

} // namespace statespec
