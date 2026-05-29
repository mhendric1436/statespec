#pragma once

#include "statespec/ir_core.hpp"

#include <cstdint>

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
    std::string resource_pattern;
    std::optional<std::string> ttl;
    std::int64_t ttl_seconds{0};
    std::optional<std::string> renew_every;
    std::int64_t renew_every_seconds{0};
    std::optional<std::string> holder;
    std::optional<bool> fencing_token;
    bool fencing_token_enabled{false};
    std::optional<std::string> max_ttl;
    std::optional<std::int64_t> max_ttl_seconds;
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
