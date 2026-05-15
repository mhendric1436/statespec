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

struct LogEvent
{
    std::string name;
    LogLevel level = LogLevel::Info;
    std::string event_name;
    Json fields = Json::object({});
};

class ILogSink
{
  public:
    virtual ~ILogSink() = default;

    virtual void emit_log(
        IBackend& backend,
        const LogEvent& event
    ) = 0;

    virtual void emit_logTx(
        ITransaction& tx,
        const LogEvent& event
    ) = 0;
};

} // namespace statespec::backend
