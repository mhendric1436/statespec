#pragma once

#include "statespec/ast_core.hpp"

namespace statespec
{

struct GarbageCollectionPolicyDecl
{
    std::optional<std::string> after;
    std::optional<std::string> mode;
    SourceRange range;
};

struct StateDecl
{
    std::string name;
    bool terminal = false;
    std::optional<GarbageCollectionPolicyDecl> garbage_collection;
    std::optional<SourceRange> duplicate_garbage_collection_range;
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

struct IndexDecl
{
    std::string name;
    std::vector<std::string> fields;
    bool unique = false;
    SourceRange range;
};

struct OwnershipDecl
{
    std::optional<std::string> authority;
    std::optional<std::string> system_of_record;
    std::optional<std::string> lifecycle;
    SourceRange range;
};

struct RelationDecl
{
    std::string kind;
    std::string name;
    std::string target;
    bool optional = false;
    std::optional<std::string> relation_kind;
    std::optional<std::string> on_parent_delete;
    std::vector<std::string> parent_must_be_in;
    std::vector<std::string> unique_within_parent;
    SourceRange range;
};

struct ChildDecl
{
    std::string name;
    std::string target_entity;
    std::string relation;
    SourceRange range;
};

struct InvariantDecl
{
    std::string name;
    std::string expression;
    SourceRange range;
};

struct EntityDecl
{
    std::string name;
    std::vector<std::string> key_fields;
    std::vector<FieldDecl> fields;
    std::vector<IndexDecl> indexes;
    std::optional<OwnershipDecl> ownership;
    std::vector<RelationDecl> relations;
    std::vector<ChildDecl> children;
    std::vector<InvariantDecl> invariants;
    std::optional<StateMachineDecl> state_machine;
    std::vector<BlockMemberOrder> member_order;
    SourceRange range;
};

} // namespace statespec
