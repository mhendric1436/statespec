#pragma once

#include "statespec/source.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

struct ImportDecl
{
    std::string name;
    std::optional<std::string> alias;
    SourceRange range;
};

struct FieldDecl
{
    std::string name;
    std::string type;
    SourceRange range;
};

struct StateDecl
{
    std::string name;
    bool terminal = false;
    SourceRange range;
};

struct TransitionDecl
{
    std::string from;
    std::string to;
    SourceRange range;
};

struct StateMachineDecl
{
    std::vector<StateDecl> states;
    std::optional<std::string> initial_state;
    std::vector<std::string> terminal_states;
    std::vector<TransitionDecl> transitions;
    SourceRange range;
};

struct EntityDecl
{
    std::string name;
    std::vector<std::string> key_fields;
    std::vector<FieldDecl> fields;
    std::optional<StateMachineDecl> state_machine;
    SourceRange range;
};

struct WorkflowStepDecl
{
    std::string name;
    std::optional<std::string> expected_execution_time;
    std::optional<int> max_retries;
    SourceRange range;
};

struct WorkflowDecl
{
    std::string name;
    std::optional<int> version;
    std::optional<bool> singleton;
    std::optional<std::string> expected_execution_time;
    std::optional<std::string> start_step;
    std::vector<WorkflowStepDecl> steps;
    SourceRange range;
};

struct SystemDecl
{
    std::string name;
    std::vector<EntityDecl> entities;
    std::vector<WorkflowDecl> workflows;
    SourceRange range;
};

struct Spec
{
    std::optional<std::string> version;
    std::vector<ImportDecl> imports;
    std::optional<SystemDecl> system;
};

} // namespace statespec
