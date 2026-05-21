#pragma once

#include "statespec/semantic_core.hpp"

namespace statespec
{

struct SemanticMessage
{
    std::string name;
    std::optional<std::string> idempotency_key;
    std::vector<SemanticField> payload_fields;
};

struct SemanticQueue
{
    std::string name;
    std::optional<std::string> namespace_name;
    std::optional<std::string> channel;
    std::optional<std::string> visibility_timeout;
    std::optional<int> max_attempts;
    std::optional<SemanticReference> dead_letter;
    std::vector<SemanticMessage> messages;
};

struct SemanticLease
{
    std::string name;
    std::optional<std::string> resource;
    std::optional<std::string> ttl;
    std::optional<std::string> renew_every;
    std::optional<std::string> holder;
    std::optional<bool> fencing_token;
    std::optional<std::string> max_ttl;
};

struct SemanticWorker
{
    std::string name;
    std::optional<bool> singleton;
    std::optional<SemanticReference> lease;
    std::optional<SemanticReference> polls;
    std::optional<SemanticReference> executes;
    std::optional<int> concurrency;
};

} // namespace statespec
