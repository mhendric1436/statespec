#pragma once

#include "backend.hpp"

#include <string>
#include <vector>

namespace statespec::backend
{

enum class MetricKind
{
    Counter,
    Gauge,
    Histogram,
};

struct MetricSample
{
    std::string name;
    MetricKind kind = MetricKind::Counter;
    std::string backend_name;
    double value = 0.0;
    std::string unit;
    Json labels = Json::object({});
};

struct MetricDefinition
{
    std::string name;
    MetricKind kind = MetricKind::Counter;
    std::string backend_name;
    std::string unit;
    std::vector<FieldDescriptor> labels;
};

struct MetricDefinitionRegistration
{
    bool registered_new = false;
    MetricDefinition definition;
};

class IMetricSink
{
  public:
    virtual ~IMetricSink() = default;

    // Registers the metric catalog entry idempotently. Re-registering an
    // identical definition is a no-op; incompatible redefinition should fail.
    virtual MetricDefinitionRegistration register_definition(
        IBackend& backend,
        const MetricDefinition& definition
    ) = 0;

    virtual MetricDefinitionRegistration register_definitionTx(
        ITransaction& tx,
        const MetricDefinition& definition
    ) = 0;

    // Transactional records are staged in the caller's OCC transaction. A
    // commit makes the sample visible to exporters; aborting drops it. Runtime
    // implementations should validate labels against the registered definition.
    virtual void record_metric(
        IBackend& backend,
        const MetricSample& sample
    ) = 0;

    virtual void record_metricTx(
        ITransaction& tx,
        const MetricSample& sample
    ) = 0;
};

} // namespace statespec::backend
