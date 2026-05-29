#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <algorithm>
#include <sstream>
#include <string_view>

namespace statespec
{

namespace
{

std::string rust_descriptor_module_declarations(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    const auto has_common_shapes = std::any_of(
        system.shapes.begin(), system.shapes.end(),
        [&](const auto& shape)
        {
            return !std::any_of(
                system.apis.begin(), system.apis.end(),
                [&](const auto& api)
                {
                    return (api.input.has_value() && *api.input == shape.name) ||
                           (api.output.has_value() && *api.output == shape.name);
                }
            );
        }
    );
    std::ostringstream out;
    out << "#[path = \"descriptors/core.rs\"]\n";
    out << "mod descriptor_core;\n";
    out << "#[path = \"descriptors/events.rs\"]\n";
    out << "mod descriptor_events;\n";
    out << "pub use descriptor_events::*;\n";
    out << "#[path = \"descriptors/external_systems.rs\"]\n";
    out << "mod descriptor_external_systems;\n";
    out << "pub use descriptor_external_systems::*;\n";
    if (has_common_shapes)
    {
        out << "#[path = \"descriptors/shapes.rs\"]\n";
        out << "mod descriptor_shapes;\n";
        out << "pub use descriptor_shapes::*;\n";
    }
    out << "#[path = \"descriptors/runtime.rs\"]\n";
    out << "mod descriptor_runtime;\n";
    auto add_runtime_registration_module = [&](bool used, std::string_view module_name)
    {
        if (!used)
        {
            return;
        }
        out << "#[path = \"descriptors/runtime/" << module_name << ".rs\"]\n";
        out << "mod descriptor_runtime_" << module_name << ";\n";
        out << "pub use descriptor_runtime_" << module_name << "::*;\n";
    };
    add_runtime_registration_module(usage.uses_feature_flags, "feature_flags");
    add_runtime_registration_module(usage.uses_queues, "queues");
    add_runtime_registration_module(usage.uses_leases, "leases");
    add_runtime_registration_module(usage.uses_logs, "logs");
    add_runtime_registration_module(usage.uses_metrics, "metrics");
    add_runtime_registration_module(usage.uses_logs && usage.uses_metrics, "observability");
    add_runtime_registration_module(usage.uses_workflows, "workflows");
    for (const auto& workflow : system.workflows)
    {
        out << "#[path = \"workflows/" << snake_identifier(workflow.name) << ".rs\"]\n";
        out << "mod workflow_" << snake_identifier(workflow.name) << ";\n";
    }
    return out.str();
}

} // namespace

std::string generate_rust_descriptor_prelude(
    const IrSystem& system,
    const std::string& external_system_runtime,
    const std::string& external_system_metadata_runtime,
    const std::string& entity_repository_runtime
)
{
    std::ostringstream out;
    out << rust_descriptor_module_declarations(system) << "\n";
    out << "use std::collections::BTreeMap;\n";
    out << "use std::time::Duration;\n\n";
    out << "use crate::backend::{Backend, BackendError, BackendResult, "
           "ConflictKind, FieldDescriptor, FieldType, Transaction, "
           "VersionedRecord};\n";
    out << "use crate::external_system::{ExternalSystemMetadataLookup, "
           "ExternalSystemMetadataResolution, "
           "ExternalSystemMetadataResolver};\n";
    out << "use crate::feature_flag::{FeatureFlagDefinition, FeatureFlagScopeKind, "
           "FeatureFlagStore, FeatureFlagType, FeatureFlagValue};\n";
    out << "use crate::lease::{LeaseDefinition, LeaseDefinitionId, LeaseStore};\n";
    out << "use crate::json::Json;\n";
    out << "use crate::log::{LogDefinition, LogLevel, LogSink};\n";
    out << "use crate::metric::{MetricDefinition, MetricKind, MetricSink};\n";
    out << "use crate::queue::{RegisterQueueDefinitionRequest, QueueDefinition, QueueStore};\n";
    out << "use crate::workflow::{RegisterWorkflowDefinitionRequest, WorkflowDefinition, "
           "WorkflowStore};\n\n";
    out << "pub use crate::descriptor_types::{ApiDescriptor, ApiRequestContext, ApiResponse, "
           "ApiRouteDescriptor, ApiServerDescriptor, WorkerContext, WorkerDescriptor};\n\n";
    out << "pub use crate::shape_types::ShapeDescriptor;\n\n";
    out << generate_rust_backend_descriptor_types(
        external_system_runtime, external_system_metadata_runtime, entity_repository_runtime
    );

    return out.str();
}

} // namespace statespec
