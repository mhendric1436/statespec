#pragma once

#include "statespec/ast_core.hpp"

namespace statespec
{

struct MessageDecl
{
    std::string name;
    std::optional<std::string> idempotency_key;
    std::vector<FieldDecl> payload_fields;
    SourceRange range;
};

struct QueueDecl
{
    std::string name;
    std::optional<std::string> namespace_name;
    std::optional<std::string> channel;
    std::optional<std::string> visibility_timeout;
    std::optional<int> max_attempts;
    std::optional<std::string> dead_letter;
    std::vector<MessageDecl> messages;
    SourceRange range;
};

struct LeaseDecl
{
    std::string name;
    std::optional<std::string> resource;
    std::optional<std::string> ttl;
    std::optional<std::string> renew_every;
    std::optional<std::string> holder;
    std::optional<bool> fencing_token;
    std::optional<std::string> max_ttl;
    SourceRange range;
};

struct WorkerDecl
{
    std::string name;
    std::optional<bool> singleton;
    std::optional<std::string> lease;
    std::optional<std::string> polls;
    std::optional<std::string> executes;
    std::optional<int> concurrency;
    SourceRange range;
};

} // namespace statespec
