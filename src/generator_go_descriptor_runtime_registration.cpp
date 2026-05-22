#include "generator_go_descriptor_areas.hpp"

#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string go_feature_flag_helpers()
{
    return R"go(func featureFlagTypeFromDescriptor(flagType string) FeatureFlagType {
	switch flagType {
	case "string":
		return FeatureFlagString
	case "int":
		return FeatureFlagInteger
	case "decimal":
		return FeatureFlagDecimal
	default:
		return FeatureFlagBool
	}
}

func featureFlagScopeFromDescriptor(scope string) FeatureFlagScopeKind {
	switch {
	case scope == "system":
		return FeatureFlagScopeSystem
	case scope == "user":
		return FeatureFlagScopeUser
	case strings.HasPrefix(scope, "entity "):
		return FeatureFlagScopeEntity
	default:
		return FeatureFlagScopeTenant
	}
}

func featureFlagValueFromDescriptor(definition FeatureFlagDescriptor) FeatureFlagValue {
	switch definition.Type {
	case "string":
		return StringFeatureFlagValue(definition.DefaultValue)
	case "int":
		value, _ := strconv.ParseInt(definition.DefaultValue, 10, 64)
		return IntegerFeatureFlagValue(value)
	case "decimal":
		value, _ := strconv.ParseFloat(definition.DefaultValue, 64)
		return DecimalFeatureFlagValue(value)
	default:
		return BoolFeatureFlagValue(definition.DefaultValue == "true")
	}
}

)go";
}

std::string go_lease_helpers()
{
    return R"go(func leaseDefinitionFromDescriptor(definition LeaseDescriptor) LeaseDefinition {
	resourcePattern := definition.Name
	if definition.Resource != nil {
		resourcePattern = *definition.Resource
	}
	renewEvery := definition.TTL
	if definition.RenewEvery != nil {
		renewEvery = *definition.RenewEvery
	}
	return LeaseDefinition{
		ID:              LeaseDefinitionID{Name: definition.Name, Version: 1},
		ResourcePattern: resourcePattern,
		TTL:             definition.TTL,
		RenewEvery:      renewEvery,
		MaxTTL:          definition.MaxTTL,
		FencingToken:    definition.FencingToken,
	}
}

)go";
}

std::string go_registration_helpers(const RuntimeDomainUsage& usage)
{
    std::ostringstream out;
    if (usage.uses_feature_flags)
    {
        out << R"go(func RegisterFeatureFlagDefinitionsTx(ctx context.Context, tx Transaction, store FeatureFlagStore) error {
	for _, definition := range FeatureFlagDefinitions() {
		_, err := store.RegisterDefinitionTx(ctx, tx, FeatureFlagDefinition{
			Name:         definition.Name,
			Type:         featureFlagTypeFromDescriptor(definition.Type),
			DefaultValue: featureFlagValueFromDescriptor(definition),
			Scope:        featureFlagScopeFromDescriptor(definition.Scope),
			Owner:        definition.Owner,
			Description:  definition.Description,
			Expires:      definition.Expires,
		})
		if err != nil {
			return err
		}
	}
	return nil
}

)go";
    }
    if (usage.uses_queues)
    {
        out << R"go(func RegisterQueueDefinitionsTx(ctx context.Context, tx Transaction, store QueueStore) error {
	for _, definition := range QueueDefinitions() {
		_, err := store.RegisterDefinitionTx(ctx, tx, RegisterQueueDefinitionRequest{Definition: definition})
		if err != nil {
			return err
		}
	}
	return nil
}

)go";
    }
    if (usage.uses_leases)
    {
        out << R"go(func RegisterLeaseDefinitionsTx(ctx context.Context, tx Transaction, store LeaseStore) error {
	for _, definition := range LeaseDefinitions() {
		_, err := store.RegisterDefinitionTx(ctx, tx, leaseDefinitionFromDescriptor(definition))
		if err != nil {
			return err
		}
	}
	return nil
}

)go";
    }
    if (usage.uses_logs)
    {
        out << R"go(func RegisterLogDefinitionsTx(ctx context.Context, tx Transaction, sink LogSink) error {
	for _, definition := range LogDefinitions() {
		_, err := sink.RegisterDefinitionTx(ctx, tx, LogSignalDefinition{
			Name:      definition.Name,
			Level:     logLevelFromDescriptor(definition.Level),
			EventName: definition.EventName,
			Fields:    definition.Fields,
		})
		if err != nil {
			return err
		}
	}
	return nil
}

)go";
    }
    if (usage.uses_metrics)
    {
        out << R"go(func RegisterMetricDefinitionsTx(ctx context.Context, tx Transaction, sink MetricSink) error {
	for _, definition := range MetricDefinitions() {
		_, err := sink.RegisterDefinitionTx(ctx, tx, MetricInstrumentDefinition{
			Name:        definition.Name,
			Kind:        metricKindFromDescriptor(definition.Kind),
			BackendName: definition.BackendName,
			Unit:        definition.Unit,
			Labels:      definition.Labels,
		})
		if err != nil {
			return err
		}
	}
	return nil
}

)go";
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        out << R"go(func RegisterObservabilityCatalogTx(ctx context.Context, tx Transaction, logSink LogSink, metricSink MetricSink) error {
	if err := RegisterLogDefinitionsTx(ctx, tx, logSink); err != nil {
		return err
	}
	return RegisterMetricDefinitionsTx(ctx, tx, metricSink)
}

)go";
    }
    if (usage.uses_workflows)
    {
        out << R"go(func RegisterWorkflowDefinitionsTx(ctx context.Context, tx Transaction, store WorkflowStore) error {
	for _, definition := range WorkflowDefinitions() {
		_, err := store.RegisterDefinitionTx(ctx, tx, RegisterWorkflowDefinitionRequest{Definition: definition})
		if err != nil {
			return err
		}
	}
	return nil
}

)go";
    }
    return out.str();
}

} // namespace

std::string generate_go_runtime_registration(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream helper;
    std::ostringstream params;
    std::ostringstream args;
    std::ostringstream calls;
    auto add = [&](std::string_view type, std::string_view name, std::string_view fn)
    {
        params << ", " << name << " " << type;
        args << ", " << name;
        calls << "\tif err := " << fn << "(ctx, tx, " << name << "); err != nil {\n";
        calls << "\t\treturn err\n";
        calls << "\t}\n";
    };
    if (usage.uses_feature_flags)
    {
        helper << go_feature_flag_helpers();
        add("FeatureFlagStore", "featureFlagStore", "RegisterFeatureFlagDefinitionsTx");
    }
    if (usage.uses_queues)
    {
        add("QueueStore", "queueStore", "RegisterQueueDefinitionsTx");
    }
    if (usage.uses_leases)
    {
        helper << go_lease_helpers();
        add("LeaseStore", "leaseStore", "RegisterLeaseDefinitionsTx");
    }
    if (usage.uses_workflows)
    {
        add("WorkflowStore", "workflowStore", "RegisterWorkflowDefinitionsTx");
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        params << ", logSink LogSink, metricSink MetricSink";
        args << ", logSink, metricSink";
        calls << "\tif err := RegisterObservabilityCatalogTx(ctx, tx, logSink, metricSink); err != "
                 "nil {\n";
        calls << "\t\treturn err\n";
        calls << "\t}\n";
    }
    else
    {
        if (usage.uses_logs)
        {
            add("LogSink", "logSink", "RegisterLogDefinitionsTx");
        }
        if (usage.uses_metrics)
        {
            add("MetricSink", "metricSink", "RegisterMetricDefinitionsTx");
        }
    }
    return templates.render(
        "generated/runtime_registration.go.tmpl",
        TemplateRenderer::Values{
            {"feature_flag_helpers", helper.str()},
            {"lease_helpers", {}},
            {"domain_registration_helpers", go_registration_helpers(usage)},
            {"tx_parameters", params.str()},
            {"runtime_parameters", params.str()},
            {"runtime_arguments", args.str()},
            {"tx_calls", calls.str()},
        }
    );
}

} // namespace statespec
