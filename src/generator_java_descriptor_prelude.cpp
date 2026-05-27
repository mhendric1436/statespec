#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

namespace
{

std::string java_descriptor_module_imports(const IrSystem& system)
{
    std::ostringstream out;
    out << "import com.statespec.generated.descriptors.CoreDescriptorModule;\n";
    out << "import com.statespec.generated.descriptors.EventDescriptorModule;\n";
    out << "import com.statespec.generated.descriptors.ExternalSystemDescriptorModule;\n";
    out << "import com.statespec.generated.descriptors.RuntimeDescriptorModule;\n";
    out << "import com.statespec.generated.descriptors.ShapeDescriptorModule;\n";
    out << "import com.statespec.generated.descriptors.WorkerDescriptorModule;\n";
    for (const auto& workflow : system.workflows)
    {
        out << "import com.statespec.generated.workflows." << pascal_identifier(workflow.name)
            << "DescriptorModule;\n";
    }
    return out.str();
}

} // namespace

std::string generate_java_descriptor_prelude(
    const IrSystem& system,
    const std::string&,
    const std::string&,
    const std::string& entity_repository_runtime
)
{
    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << java_descriptor_module_imports(system);
    out << "import com.statespec.backend.Json;\n";
    out << "import com.statespec.backend.Queue;\n";
    out << "import com.statespec.backend.Workflow;\n";
    out << "import com.statespec.backend.Backend.FieldDescriptor;\n";
    out << "import com.statespec.backend.Backend.FieldType;\n";
    out << "import com.statespec.backend.Queue.QueueDefinition;\n";
    out << "import com.statespec.backend.Workflow.WorkflowDefinition;\n";
    out << "import java.time.Duration;\n";
    out << "import java.util.List;\n";
    out << "import java.util.Map;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class Descriptors {\n";
    out << "    private Descriptors() {}\n\n";
    out << "    public record EventEnvelope(\n";
    out << "        String name,\n";
    out << "        Map<String, Json> fields\n";
    out << "    ) {}\n\n";
    for (const auto& event : system.events)
    {
        out << "    public static EventEnvelope build" << pascal_identifier(event.name)
            << "Event(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "        Json " << lower_camel_identifier(field.name);
            out << (i + 1 < event.fields.size() ? "," : "") << "\n";
        }
        out << "    ) {\n";
        out << "        return EventDescriptorModule.build" << pascal_identifier(event.name)
            << "Event(";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << lower_camel_identifier(event.fields[i].name);
        }
        out << ");\n";
        out << "    }\n\n";
    }

    out << "    public record LeaseDefinition(\n";
    out << "        String name,\n";
    out << "        Optional<String> resource,\n";
    out << "        Duration ttl,\n";
    out << "        Optional<Duration> renewEvery,\n";
    out << "        Optional<String> holder,\n";
    out << "        boolean fencingToken,\n";
    out << "        Optional<Duration> maxTtl\n";
    out << "    ) {}\n\n";
    out << "    public record FeatureFlagDefinition(\n";
    out << "        String name,\n";
    out << "        String type,\n";
    out << "        String defaultValue,\n";
    out << "        String scope,\n";
    out << "        Optional<String> owner,\n";
    out << "        Optional<String> description,\n";
    out << "        Optional<String> expires\n";
    out << "    ) {}\n\n";
    out << "    public record ValueDescriptor(\n";
    out << "        String name,\n";
    out << "        String type,\n";
    out << "        Optional<String> constraint\n";
    out << "    ) {}\n\n";
    out << "    public record EnumMemberDescriptor(\n";
    out << "        String name,\n";
    out << "        Optional<String> value\n";
    out << "    ) {}\n\n";
    out << "    public record EnumDescriptor(\n";
    out << "        String name,\n";
    out << "        List<EnumMemberDescriptor> members\n";
    out << "    ) {}\n\n";
    out << "    public record EventDescriptor(\n";
    out << "        String name,\n";
    out << "        List<FieldDescriptor> fields\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemPropertyDescriptor(\n";
    out << "        String name,\n";
    out << "        String value\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemMetadataMappingDescriptor(\n";
    out << "        String source,\n";
    out << "        String target\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemMetadataDescriptor(\n";
    out << "        String entity,\n";
    out << "        Optional<String> tenantField,\n";
    out << "        String profileField,\n";
    out << "        List<String> keyFields,\n";
    out << "        List<String> requiredFields,\n";
    out << "        List<ExternalSystemMetadataMappingDescriptor> mappings\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemDescriptor(\n";
    out << "        String name,\n";
    out << "        List<ExternalSystemPropertyDescriptor> properties,\n";
    out << "        Optional<ExternalSystemMetadataDescriptor> metadata\n";
    out << "    ) {}\n\n";
    out << "    public record ApiDescriptor(\n";
    out << "        String name,\n";
    out << "        Optional<String> method,\n";
    out << "        Optional<String> path,\n";
    out << "        Optional<String> input,\n";
    out << "        Optional<String> output,\n";
    out << "        Optional<String> error,\n";
    out << "        Optional<String> startsWorkflow,\n";
    out << "        Optional<String> enqueues\n";
    out << "    ) {}\n\n";
    out << "    public record ApiServerDescriptor(\n";
    out << "        String name,\n";
    out << "        List<String> serves,\n";
    out << "        int concurrency\n";
    out << "    ) {}\n\n";
    out << "    public record ApiRouteDescriptor(\n";
    out << "        String name,\n";
    out << "        String serverName,\n";
    out << "        String apiName,\n";
    out << "        Optional<String> method,\n";
    out << "        Optional<String> path,\n";
    out << "        Optional<String> input,\n";
    out << "        Optional<String> output,\n";
    out << "        Optional<String> error\n";
    out << "    ) {}\n\n";
    out << "    public record WorkerDescriptor(\n";
    out << "        String name,\n";
    out << "        boolean singleton,\n";
    out << "        Optional<String> lease,\n";
    out << "        Optional<String> polls,\n";
    out << "        Optional<String> executes,\n";
    out << "        int concurrency\n";
    out << "    ) {}\n\n";
    out << "    public record WorkerContext(\n";
    out << "        String workerName,\n";
    out << "        boolean singleton,\n";
    out << "        Optional<String> lease,\n";
    out << "        Optional<String> polls,\n";
    out << "        Optional<String> executes,\n";
    out << "        int concurrency\n";
    out << "    ) {}\n\n";
    out << "    public record PolicyRuleDescriptor(\n";
    out << "        String action,\n";
    out << "        String condition\n";
    out << "    ) {}\n\n";
    out << "    public record QuotaDescriptor(\n";
    out << "        String name,\n";
    out << "        String expression\n";
    out << "    ) {}\n\n";
    out << "    public record PolicyDescriptor(\n";
    out << "        String name,\n";
    out << "        Optional<String> tenantScopedBy,\n";
    out << "        List<PolicyRuleDescriptor> allows,\n";
    out << "        List<PolicyRuleDescriptor> denies,\n";
    out << "        List<QuotaDescriptor> quotas,\n";
    out << "        List<String> audits\n";
    out << "    ) {}\n\n";
    out << "    public record ShapeDescriptor(\n";
    out << "        String name,\n";
    out << "        List<FieldDescriptor> fields\n";
    out << "    ) {}\n\n";
    out << "    public record LogDefinition(\n";
    out << "        String name,\n";
    out << "        String level,\n";
    out << "        String eventName,\n";
    out << "        List<FieldDescriptor> fields\n";
    out << "    ) {}\n\n";
    out << "    public record MetricDefinition(\n";
    out << "        String name,\n";
    out << "        String kind,\n";
    out << "        String backendName,\n";
    out << "        String unit,\n";
    out << "        List<FieldDescriptor> labels\n";
    out << "    ) {}\n\n";
    out << "    public record GarbageCollectionPolicy(\n";
    out << "        String after,\n";
    out << "        String mode\n";
    out << "    ) {}\n\n";
    out << "    public record EntityStateDescriptor(\n";
    out << "        String name,\n";
    out << "        boolean terminal,\n";
    out << "        Optional<GarbageCollectionPolicy> garbageCollection\n";
    out << "    ) {}\n\n";
    out << "    public record EntityOwnershipDescriptor(\n";
    out << "        String authority,\n";
    out << "        String systemOfRecord,\n";
    out << "        String lifecycle\n";
    out << "    ) {}\n\n";
    out << "    public record EntityRelationDescriptor(\n";
    out << "        String kind,\n";
    out << "        String name,\n";
    out << "        String target,\n";
    out << "        boolean optional,\n";
    out << "        Optional<String> relationKind,\n";
    out << "        Optional<String> onParentDelete,\n";
    out << "        List<String> parentMustBeIn,\n";
    out << "        List<String> uniqueWithinParent\n";
    out << "    ) {}\n\n";
    out << "    public record EntityChildDescriptor(\n";
    out << "        String name,\n";
    out << "        String targetEntity,\n";
    out << "        String relation\n";
    out << "    ) {}\n\n";
    out << "    public record EntityInvariantDescriptor(\n";
    out << "        String name,\n";
    out << "        String expression\n";
    out << "    ) {}\n\n";
    out << "    public record EntityDescriptor(\n";
    out << "        String name,\n";
    out << "        List<String> keyFields,\n";
    out << "        Optional<EntityOwnershipDescriptor> ownership,\n";
    out << "        List<EntityRelationDescriptor> relations,\n";
    out << "        List<EntityChildDescriptor> children,\n";
    out << "        List<EntityInvariantDescriptor> invariants,\n";
    out << "        List<EntityStateDescriptor> states,\n";
    out << "        Optional<String> initialState,\n";
    out << "        List<String> terminalStates\n";
    out << "    ) {}\n\n";

    out << entity_repository_runtime << "\n";

    return out.str();
}

} // namespace statespec
