#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_descriptor_prelude(const IrSystem& system)
{
    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << "import com.statespec.backend.Backend;\n";
    out << "import com.statespec.backend.ExternalSystem;\n";
    out << "import com.statespec.backend.FeatureFlag;\n";
    out << "import com.statespec.backend.Json;\n";
    out << "import com.statespec.backend.Lease;\n";
    out << "import com.statespec.backend.Log;\n";
    out << "import com.statespec.backend.Metric;\n";
    out << "import com.statespec.backend.Queue;\n";
    out << "import com.statespec.backend.Workflow;\n";
    out << "import com.statespec.backend.Backend.CollectionDescriptor;\n";
    out << "import com.statespec.backend.Backend.FieldDescriptor;\n";
    out << "import com.statespec.backend.Backend.FieldType;\n";
    out << "import com.statespec.backend.Backend.IndexDescriptor;\n";
    out << "import com.statespec.backend.Queue.QueueDefinition;\n";
    out << "import com.statespec.backend.Workflow.WorkflowDefinition;\n";
    out << "import com.statespec.backend.Workflow.WorkflowStepDefinition;\n";
    out << "import java.time.Duration;\n";
    out << "import java.util.List;\n";
    out << "import java.util.Locale;\n";
    out << "import java.util.Map;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class Descriptors {\n";
    out << "    private Descriptors() {}\n\n";
    out << "    public record EventEnvelope(\n";
    out << "        String name,\n";
    out << "        Map<String, Json> fields\n";
    out << "    ) {}\n\n";
    for (const auto& shape : system.shapes)
    {
        out << "    public record " << pascal_identifier(shape.name) << "(\n";
        for (std::size_t i = 0; i < shape.fields.size(); ++i)
        {
            const auto& field = shape.fields[i];
            out << "        " << java_shape_type(field.type) << " " << field.name;
            out << (i + 1 < shape.fields.size() ? "," : "") << "\n";
        }
        out << "    ) {}\n\n";
    }

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
        out << "        return new EventEnvelope(\n";
        out << "            " << java_string(event.name) << ",\n";
        out << "            Map.of(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "                " << java_string(field.name) << ", "
                << lower_camel_identifier(field.name);
            out << (i + 1 < event.fields.size() ? "," : "") << "\n";
        }
        out << "            )\n";
        out << "        );\n";
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
    out << "    public record ExternalSystemMetadataMappingAssignment(\n";
    out << "        String source,\n";
    out << "        String sourceRoot,\n";
    out << "        String sourceField,\n";
    out << "        String targetRoot,\n";
    out << "        String field\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemMetadataMappingPlan(\n";
    out << "        List<ExternalSystemMetadataMappingAssignment> allMappings,\n";
    out << "        List<ExternalSystemMetadataMappingAssignment> clientMappings,\n";
    out << "        List<ExternalSystemMetadataMappingAssignment> requestMappings\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemMetadataMissingMappingSource(\n";
    out << "        String source,\n";
    out << "        String sourceRoot,\n";
    out << "        String sourceField,\n";
    out << "        String targetRoot,\n";
    out << "        String field\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemMetadataMappingInputs(\n";
    out << "        Map<String, Json> input,\n";
    out << "        Map<String, Json> entity,\n";
    out << "        Map<String, Json> workflow,\n";
    out << "        Map<String, Json> metadata\n";
    out << "    ) {\n";
    out << "        public Optional<Json> sourceValue(String sourceRoot, String sourceField) "
           "{\n";
    out << "            Map<String, Json> values = switch (sourceRoot) {\n";
    out << "                case \"input\" -> input;\n";
    out << "                case \"entity\" -> entity;\n";
    out << "                case \"workflow\" -> workflow;\n";
    out << "                case \"metadata\" -> metadata;\n";
    out << "                default -> null;\n";
    out << "            };\n";
    out << "            if (values == null) {\n";
    out << "                return Optional.empty();\n";
    out << "            }\n";
    out << "            return Optional.ofNullable(values.get(sourceField));\n";
    out << "        }\n\n";
    out << "        public Optional<Json> assignmentValue(\n";
    out << "            ExternalSystemMetadataMappingAssignment assignment\n";
    out << "        ) {\n";
    out << "            return sourceValue(assignment.sourceRoot(), "
           "assignment.sourceField());\n";
    out << "        }\n";
    out << "    }\n\n";
    out << "    public record ExternalSystemMetadataMappingOutput(\n";
    out << "        Map<String, Json> clientConfig,\n";
    out << "        Map<String, Json> requestPayload,\n";
    out << "        List<ExternalSystemMetadataMissingMappingSource> missingSources\n";
    out << "    ) {}\n\n";
    out << "    public static List<ExternalSystemMetadataMissingMappingSource> "
           "missingExternalSystemMetadataMappingSources(\n";
    out << "        ExternalSystemMetadataMappingPlan plan,\n";
    out << "        ExternalSystemMetadataMappingInputs inputs\n";
    out << "    ) {\n";
    out << "        java.util.ArrayList<ExternalSystemMetadataMissingMappingSource> "
           "missing = new java.util.ArrayList<>();\n";
    out << "        for (ExternalSystemMetadataMappingAssignment assignment : "
           "plan.allMappings()) {\n";
    out << "            if (inputs.assignmentValue(assignment).isEmpty()) {\n";
    out << "                missing.add(new ExternalSystemMetadataMissingMappingSource(\n";
    out << "                    assignment.source(),\n";
    out << "                    assignment.sourceRoot(),\n";
    out << "                    assignment.sourceField(),\n";
    out << "                    assignment.targetRoot(),\n";
    out << "                    assignment.field()\n";
    out << "                ));\n";
    out << "            }\n";
    out << "        }\n";
    out << "        return List.copyOf(missing);\n";
    out << "    }\n\n";
    out << "    public interface ExternalSystemMetadataMappingApplicator {\n";
    out << "        ExternalSystemMetadataMappingOutput applyExternalSystemMetadataMappings(\n";
    out << "            ExternalSystemMetadataMappingPlan plan,\n";
    out << "            ExternalSystemMetadataMappingInputs inputs\n";
    out << "        ) throws Exception;\n";
    out << "    }\n\n";
    out << "    public record ExternalSystemMetadataDescriptor(\n";
    out << "        String entity,\n";
    out << "        Optional<String> tenantField,\n";
    out << "        String profileField,\n";
    out << "        List<String> keyFields,\n";
    out << "        List<String> requiredFields,\n";
    out << "        List<ExternalSystemMetadataMappingDescriptor> mappings\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemOperatorMetadataUpsertRequest(\n";
    out << "        ExternalSystem.MetadataLookup lookup,\n";
    out << "        Json document,\n";
    out << "        Optional<Long> expectedVersion\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemOperatorMetadataGetRequest(\n";
    out << "        ExternalSystem.MetadataLookup lookup\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemOperatorMetadataDisableRequest(\n";
    out << "        ExternalSystem.MetadataLookup lookup,\n";
    out << "        Optional<Long> expectedVersion,\n";
    out << "        String disabledStatus\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemOperatorMetadataDeleteRequest(\n";
    out << "        ExternalSystem.MetadataLookup lookup,\n";
    out << "        Optional<Long> expectedVersion,\n";
    out << "        String deletedStatus\n";
    out << "    ) {}\n\n";
    out << "    public interface ExternalSystemOperatorMetadataRepository {\n";
    out << "        Optional<Backend.VersionedRecord> upsertMetadataTx(\n";
    out << "            Backend.Transaction tx,\n";
    out << "            ExternalSystemOperatorMetadataUpsertRequest request\n";
    out << "        ) throws Backend.BackendException;\n";
    out << "        Optional<Backend.VersionedRecord> getMetadataTx(\n";
    out << "            Backend.Transaction tx,\n";
    out << "            ExternalSystemOperatorMetadataGetRequest request\n";
    out << "        ) throws Backend.BackendException;\n";
    out << "        Optional<Backend.VersionedRecord> disableMetadataTx(\n";
    out << "            Backend.Transaction tx,\n";
    out << "            ExternalSystemOperatorMetadataDisableRequest request\n";
    out << "        ) throws Backend.BackendException;\n";
    out << "        Optional<Backend.VersionedRecord> deleteMetadataTx(\n";
    out << "            Backend.Transaction tx,\n";
    out << "            ExternalSystemOperatorMetadataDeleteRequest request\n";
    out << "        ) throws Backend.BackendException;\n";
    out << "    }\n\n";
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
    out << "    public record ApiRequestContext(\n";
    out << "        String serverName,\n";
    out << "        String apiName,\n";
    out << "        Optional<String> method,\n";
    out << "        Optional<String> path,\n";
    out << "        Json body\n";
    out << "    ) {}\n\n";
    out << "    public record ApiResponse(\n";
    out << "        int statusCode,\n";
    out << "        Json body\n";
    out << "    ) {}\n\n";
    out << "    public interface ExternalSystemOperatorMetadataApiHandler {\n";
    out << "        ApiResponse handleUpsertMetadataTx(\n";
    out << "            Backend.Transaction tx,\n";
    out << "            ExternalSystemOperatorMetadataRepository repository,\n";
    out << "            ExternalSystemOperatorMetadataUpsertRequest request\n";
    out << "        ) throws Exception;\n";
    out << "        ApiResponse handleGetMetadataTx(\n";
    out << "            Backend.Transaction tx,\n";
    out << "            ExternalSystemOperatorMetadataRepository repository,\n";
    out << "            ExternalSystemOperatorMetadataGetRequest request\n";
    out << "        ) throws Exception;\n";
    out << "        ApiResponse handleDisableMetadataTx(\n";
    out << "            Backend.Transaction tx,\n";
    out << "            ExternalSystemOperatorMetadataRepository repository,\n";
    out << "            ExternalSystemOperatorMetadataDisableRequest request\n";
    out << "        ) throws Exception;\n";
    out << "        ApiResponse handleDeleteMetadataTx(\n";
    out << "            Backend.Transaction tx,\n";
    out << "            ExternalSystemOperatorMetadataRepository repository,\n";
    out << "            ExternalSystemOperatorMetadataDeleteRequest request\n";
    out << "        ) throws Exception;\n";
    out << "    }\n\n";
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
    out << "    public interface Worker {\n";
    out << "        void run(WorkerContext context) throws Exception;\n";
    out << "    }\n\n";
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

    return out.str();
}

} // namespace statespec
