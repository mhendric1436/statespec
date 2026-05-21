#pragma once

#include "statespec/ir_core.hpp"

namespace statespec
{

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

struct IrTransition
{
    std::string from;
    std::string to;
};

struct IrIndex
{
    std::string name;
    std::vector<std::string> fields;
    bool unique = false;
};

struct IrOwnership
{
    std::string authority;
    std::string system_of_record;
    std::string lifecycle;
};

struct IrRelation
{
    std::string kind;
    std::string name;
    std::string target;
    bool optional = false;
    std::optional<std::string> relation_kind;
    std::optional<std::string> on_parent_delete;
    std::vector<std::string> parent_must_be_in;
    std::vector<std::string> unique_within_parent;
};

struct IrChild
{
    std::string name;
    std::string target_entity;
    std::string relation;
};

struct IrInvariant
{
    std::string name;
    std::string expression;
};

struct IrEntity
{
    std::string name;
    std::vector<std::string> key_fields;
    std::vector<IrField> fields;
    std::vector<IrIndex> indexes;
    std::optional<IrOwnership> ownership;
    std::vector<IrRelation> relations;
    std::vector<IrChild> children;
    std::vector<IrInvariant> invariants;
    std::vector<IrState> states;
    std::optional<std::string> initial_state;
    std::vector<std::string> terminal_states;
    std::vector<IrTransition> transitions;
};

} // namespace statespec
