#pragma once

#include "statespec/ir_core.hpp"

namespace statespec
{

struct IrMessage
{
    std::string name;
    std::optional<std::string> idempotency_key;
    std::vector<IrField> payload_fields;
};

struct IrQueue
{
    std::string name;
    std::optional<std::string> namespace_name;
    std::optional<std::string> channel;
    std::optional<std::string> visibility_timeout;
    std::optional<int> max_attempts;
    std::optional<std::string> dead_letter;
    std::vector<IrMessage> messages;
};

struct IrLease
{
    std::string name;
    std::optional<std::string> resource;
    std::optional<std::string> ttl;
    std::optional<std::string> renew_every;
    std::optional<std::string> holder;
    std::optional<bool> fencing_token;
    std::optional<std::string> max_ttl;
};

struct IrWorker
{
    std::string name;
    std::optional<bool> singleton;
    std::optional<std::string> lease;
    std::optional<std::string> polls;
    std::optional<std::string> executes;
    std::optional<int> concurrency;
};

} // namespace statespec
