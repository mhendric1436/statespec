#pragma once

#include "statespec/ir_core.hpp"

namespace statespec
{

enum class IrLogLevel
{
    Debug,
    Info,
    Warn,
    Error,
};

struct IrLog
{
    std::string name;
    std::string level;
    IrLogLevel level_kind{IrLogLevel::Info};
    std::string event_name;
    std::vector<IrField> fields;
};

enum class IrMetricKind
{
    Counter,
    Gauge,
    Histogram,
};

struct IrMetric
{
    std::string name;
    std::string kind;
    IrMetricKind kind_value{IrMetricKind::Counter};
    std::string backend_name;
    std::string unit;
    std::vector<IrField> labels;
};

} // namespace statespec
