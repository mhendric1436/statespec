#pragma once

#include "statespec/semantic_core.hpp"

namespace statespec
{

struct SemanticGarbageCollectionPolicy
{
    std::string after;
    std::string mode;
};

struct SemanticState
{
    std::string name;
    bool terminal = false;
    std::optional<SemanticGarbageCollectionPolicy> garbage_collection;
};

struct SemanticTransition
{
    std::string from;
    std::string to;
};

struct SemanticIndex
{
    std::string name;
    std::vector<std::string> fields;
    bool unique = false;
};

struct SemanticOwnership
{
    std::string authority;
    std::string system_of_record;
    std::string lifecycle;
};

struct SemanticRelation
{
    std::string kind;
    std::string name;
    SemanticReference target;
    bool optional = false;
    std::optional<std::string> relation_kind;
    std::optional<std::string> on_parent_delete;
    std::vector<std::string> parent_must_be_in;
    std::vector<std::string> unique_within_parent;
};

struct SemanticChild
{
    std::string name;
    SemanticReference target_entity;
    std::string relation;
};

struct SemanticInvariant
{
    std::string name;
    std::string expression;
};

struct SemanticEntityApiCreate
{
    std::optional<std::string> name;
    std::vector<std::string> fields;
    std::vector<std::string> response_fields;
};

struct SemanticEntityApiOperation
{
    std::optional<std::string> name;
    std::vector<std::string> response_fields;
};

struct SemanticEntityApiList
{
    std::optional<std::string> name;
    std::optional<std::string> path;
    std::vector<std::string> by;
    std::vector<std::string> response_fields;
};

struct SemanticEntityApi
{
    std::optional<std::string> resource;
    std::optional<SemanticEntityApiCreate> create;
    std::optional<SemanticEntityApiOperation> get;
    std::vector<SemanticEntityApiList> lists;
    std::optional<SemanticEntityApiOperation> update_status;
    std::optional<SemanticEntityApiOperation> delete_;
};

struct SemanticEntity
{
    std::string name;
    std::vector<std::string> key_fields;
    std::vector<SemanticField> fields;
    std::vector<SemanticIndex> indexes;
    std::optional<SemanticOwnership> ownership;
    std::vector<SemanticRelation> relations;
    std::vector<SemanticChild> children;
    std::vector<SemanticInvariant> invariants;
    std::vector<SemanticState> states;
    std::optional<std::string> initial_state;
    std::vector<std::string> terminal_states;
    std::vector<SemanticTransition> transitions;
    std::optional<SemanticEntityApi> api;
};

} // namespace statespec
