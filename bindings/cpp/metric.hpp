#pragma once

#include "backend.hpp"

#include <string>

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

class IMetricSink
{
  public:
    virtual ~IMetricSink() = default;

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
