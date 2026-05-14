#pragma once

#include "statespec/ast.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

struct IrField
{
    std::string name;
    std::string type;
};

struct IrGarbageCollectionPolicy
{
    std::string after;
    std::string mode;
};

struct IrState
{
    std::string name;
    bool terminal = false;
    std::optional<IrGarbageCollectionPolicy> garbage_collection;
};

struct IrEntity
{
    std::string name;
    std::vector<std::string> key_fields;
    std::vector<IrField> fields;
    std::vector<IrState> states;
    std::optional<std::string> initial_state;
    std::vector<std::string> terminal_states;
};

struct IrQueue
{
    std::string name;
    std::optional<std::string> channel;
    std::optional<std::string> visibility_timeout;
    std::optional<int> max_attempts;
    std::optional<std::string> dead_letter;
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

struct IrWorkflowStep
{
    std::string name;
    std::optional<std::string> expected_execution_time;
    std::optional<int> max_retries;
};

struct IrWorkflow
{
    std::string name;
    std::optional<int> version;
    std::optional<bool> singleton;
    std::optional<std::string> expected_execution_time;
    std::optional<std::string> start_step;
    std::vector<IrWorkflowStep> steps;
};

struct IrFeatureFlag
{
    std::string name;
    std::string type;
    std::string default_value;
    std::string scope;
    std::optional<std::string> owner;
    std::optional<std::string> description;
    std::optional<std::string> expires;
};

struct IrSystem
{
    std::string name;
    std::vector<IrFeatureFlag> feature_flags;
    std::vector<IrEntity> entities;
    std::vector<IrQueue> queues;
    std::vector<IrLease> leases;
    std::vector<IrWorkflow> workflows;
};

IrSystem lower_to_ir(const Spec& spec);

} // namespace statespec
