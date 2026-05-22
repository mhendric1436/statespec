#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_runtime_registration(const IrSystem&)
{
    std::ostringstream out;
    out << "func featureFlagTypeFromDescriptor(flagType string) FeatureFlagType {\n";
    out << "\tswitch flagType {\n";
    out << "\tcase \"string\":\n";
    out << "\t\treturn FeatureFlagString\n";
    out << "\tcase \"int\":\n";
    out << "\t\treturn FeatureFlagInteger\n";
    out << "\tcase \"decimal\":\n";
    out << "\t\treturn FeatureFlagDecimal\n";
    out << "\tdefault:\n";
    out << "\t\treturn FeatureFlagBool\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func featureFlagScopeFromDescriptor(scope string) FeatureFlagScopeKind {\n";
    out << "\tswitch {\n";
    out << "\tcase scope == \"system\":\n";
    out << "\t\treturn FeatureFlagScopeSystem\n";
    out << "\tcase scope == \"user\":\n";
    out << "\t\treturn FeatureFlagScopeUser\n";
    out << "\tcase strings.HasPrefix(scope, \"entity \"):\n";
    out << "\t\treturn FeatureFlagScopeEntity\n";
    out << "\tdefault:\n";
    out << "\t\treturn FeatureFlagScopeTenant\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func featureFlagValueFromDescriptor(definition FeatureFlagDescriptor) FeatureFlagValue "
           "{\n";
    out << "\tswitch definition.Type {\n";
    out << "\tcase \"string\":\n";
    out << "\t\treturn StringFeatureFlagValue(definition.DefaultValue)\n";
    out << "\tcase \"int\":\n";
    out << "\t\tvalue, _ := strconv.ParseInt(definition.DefaultValue, 10, 64)\n";
    out << "\t\treturn IntegerFeatureFlagValue(value)\n";
    out << "\tcase \"decimal\":\n";
    out << "\t\tvalue, _ := strconv.ParseFloat(definition.DefaultValue, 64)\n";
    out << "\t\treturn DecimalFeatureFlagValue(value)\n";
    out << "\tdefault:\n";
    out << "\t\treturn BoolFeatureFlagValue(definition.DefaultValue == \"true\")\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func leaseDefinitionFromDescriptor(definition LeaseDescriptor) LeaseDefinition {\n";
    out << "\tresourcePattern := definition.Name\n";
    out << "\tif definition.Resource != nil {\n";
    out << "\t\tresourcePattern = *definition.Resource\n";
    out << "\t}\n";
    out << "\trenewEvery := definition.TTL\n";
    out << "\tif definition.RenewEvery != nil {\n";
    out << "\t\trenewEvery = *definition.RenewEvery\n";
    out << "\t}\n";
    out << "\treturn LeaseDefinition{\n";
    out << "\t\tID: LeaseDefinitionID{Name: definition.Name, Version: 1},\n";
    out << "\t\tResourcePattern: resourcePattern,\n";
    out << "\t\tTTL: definition.TTL,\n";
    out << "\t\tRenewEvery: renewEvery,\n";
    out << "\t\tMaxTTL: definition.MaxTTL,\n";
    out << "\t\tFencingToken: definition.FencingToken,\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func EnsureSystemCollections(ctx context.Context, backend Backend) error {\n";
    out << "\treturn backend.EnsureCollections(ctx, CollectionDescriptors())\n";
    out << "}\n\n";

    out << "func RegisterFeatureFlagDefinitionsTx(ctx context.Context, tx Transaction, store "
           "FeatureFlagStore) error {\n";
    out << "\tfor _, definition := range FeatureFlagDefinitions() {\n";
    out << "\t\t_, err := store.RegisterDefinitionTx(ctx, tx, FeatureFlagDefinition{\n";
    out << "\t\t\tName: definition.Name,\n";
    out << "\t\t\tType: featureFlagTypeFromDescriptor(definition.Type),\n";
    out << "\t\t\tDefaultValue: featureFlagValueFromDescriptor(definition),\n";
    out << "\t\t\tScope: featureFlagScopeFromDescriptor(definition.Scope),\n";
    out << "\t\t\tOwner: definition.Owner,\n";
    out << "\t\t\tDescription: definition.Description,\n";
    out << "\t\t\tExpires: definition.Expires,\n";
    out << "\t\t})\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func RegisterQueueDefinitionsTx(ctx context.Context, tx Transaction, store QueueStore) "
           "error {\n";
    out << "\tfor _, definition := range QueueDefinitions() {\n";
    out << "\t\t_, err := store.RegisterDefinitionTx(\n";
    out << "\t\t\tctx,\n";
    out << "\t\t\ttx,\n";
    out << "\t\t\tRegisterQueueDefinitionRequest{Definition: definition},\n";
    out << "\t\t)\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func RegisterLeaseDefinitionsTx(ctx context.Context, tx Transaction, store LeaseStore) "
           "error {\n";
    out << "\tfor _, definition := range LeaseDefinitions() {\n";
    out << "\t\t_, err := store.RegisterDefinitionTx(ctx, tx, "
           "leaseDefinitionFromDescriptor(definition))\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func RegisterLogDefinitionsTx(ctx context.Context, tx Transaction, sink LogSink) error "
           "{\n";
    out << "\tfor _, definition := range LogDefinitions() {\n";
    out << "\t\t_, err := sink.RegisterDefinitionTx(ctx, tx, LogSignalDefinition{\n";
    out << "\t\t\tName: definition.Name,\n";
    out << "\t\t\tLevel: logLevelFromDescriptor(definition.Level),\n";
    out << "\t\t\tEventName: definition.EventName,\n";
    out << "\t\t\tFields: definition.Fields,\n";
    out << "\t\t})\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func RegisterMetricDefinitionsTx(ctx context.Context, tx Transaction, sink MetricSink) "
           "error {\n";
    out << "\tfor _, definition := range MetricDefinitions() {\n";
    out << "\t\t_, err := sink.RegisterDefinitionTx(ctx, tx, MetricInstrumentDefinition{\n";
    out << "\t\t\tName: definition.Name,\n";
    out << "\t\t\tKind: metricKindFromDescriptor(definition.Kind),\n";
    out << "\t\t\tBackendName: definition.BackendName,\n";
    out << "\t\t\tUnit: definition.Unit,\n";
    out << "\t\t\tLabels: definition.Labels,\n";
    out << "\t\t})\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func RegisterObservabilityCatalogTx(ctx context.Context, tx Transaction, logSink "
           "LogSink, metricSink MetricSink) error {\n";
    out << "\tif err := RegisterLogDefinitionsTx(ctx, tx, logSink); err != nil {\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\treturn RegisterMetricDefinitionsTx(ctx, tx, metricSink)\n";
    out << "}\n\n";

    out << "func RegisterWorkflowDefinitionsTx(ctx context.Context, tx Transaction, store "
           "WorkflowStore) error {\n";
    out << "\tfor _, definition := range WorkflowDefinitions() {\n";
    out << "\t\t_, err := store.RegisterDefinitionTx(ctx, tx, "
           "RegisterWorkflowDefinitionRequest{Definition: definition})\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func RegisterRuntimeCatalogTx(ctx context.Context, tx Transaction, featureFlagStore "
           "FeatureFlagStore, queueStore QueueStore, leaseStore LeaseStore, workflowStore "
           "WorkflowStore, logSink LogSink, metricSink MetricSink) error {\n";
    out << "\tif err := RegisterFeatureFlagDefinitionsTx(ctx, tx, featureFlagStore); err != nil "
           "{\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\tif err := RegisterQueueDefinitionsTx(ctx, tx, queueStore); err != nil {\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\tif err := RegisterLeaseDefinitionsTx(ctx, tx, leaseStore); err != nil {\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\tif err := RegisterWorkflowDefinitionsTx(ctx, tx, workflowStore); err != nil {\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\treturn RegisterObservabilityCatalogTx(ctx, tx, logSink, metricSink)\n";
    out << "}\n";

    out << "\nfunc RegisterRuntimeCatalog(ctx context.Context, backend Backend, "
           "featureFlagStore FeatureFlagStore, queueStore QueueStore, leaseStore LeaseStore, "
           "workflowStore WorkflowStore, logSink LogSink, metricSink MetricSink) error {\n";
    out << "\ttx, err := backend.Begin(ctx)\n";
    out << "\tif err != nil {\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\tif err := RegisterRuntimeCatalogTx(ctx, tx, featureFlagStore, queueStore, "
           "leaseStore, workflowStore, logSink, metricSink); err != nil {\n";
    out << "\t\t_ = tx.Abort(ctx)\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\treturn backend.Commit(ctx, tx)\n";
    out << "}\n";

    out << "\nfunc BootstrapRuntimeCatalog(ctx context.Context, backend Backend, "
           "featureFlagStore FeatureFlagStore, queueStore QueueStore, leaseStore LeaseStore, "
           "workflowStore WorkflowStore, logSink LogSink, metricSink MetricSink) error {\n";
    out << "\tif err := EnsureSystemCollections(ctx, backend); err != nil {\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\treturn RegisterRuntimeCatalog(ctx, backend, featureFlagStore, queueStore, "
           "leaseStore, workflowStore, logSink, metricSink)\n";
    out << "}\n";
    return out.str();
}

} // namespace statespec
