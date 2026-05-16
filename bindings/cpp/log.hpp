#pragma once

#include "backend.hpp"

#include <string>
#include <vector>

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

struct LogDefinition
{
    std::string name;
    LogLevel level = LogLevel::Info;
    std::string event_name;
    std::vector<FieldDescriptor> fields;
};

struct LogDefinitionRegistration
{
    bool registered_new = false;
    LogDefinition definition;
};

class ILogSink
{
  public:
    virtual ~ILogSink() = default;

    // Registers the log catalog entry idempotently. Re-registering an
    // identical definition is a no-op; incompatible redefinition should fail.
    virtual LogDefinitionRegistration register_definition(
        IBackend& backend,
        const LogDefinition& definition
    ) = 0;

    virtual LogDefinitionRegistration register_definitionTx(
        ITransaction& tx,
        const LogDefinition& definition
    ) = 0;

    // Transactional emits are staged in the caller's OCC transaction. A commit
    // makes the log visible to exporters; aborting the transaction drops it.
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
