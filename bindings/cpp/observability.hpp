#pragma once

#include "backend.hpp"

#include <string>

namespace statespec::backend
{

enum class LogLevel
{
    Debug,
    Info,
    Warn,
    Error,
};

enum class MetricKind
{
    Counter,
    Gauge,
    Histogram,
};

struct LogEvent
{
    std::string name;
    LogLevel level = LogLevel::Info;
    std::string event_name;
    Json fields = Json::object({});
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

class IObservabilitySink
{
  public:
    virtual ~IObservabilitySink() = default;

    virtual void emit_log(
        IBackend& backend,
        const LogEvent& event
    ) = 0;

    virtual void emit_logTx(
        ITransaction& tx,
        const LogEvent& event
    ) = 0;

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
