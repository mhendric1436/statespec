#pragma once

#include "statespec/ast_core.hpp"

namespace statespec
{

struct ValueDecl
{
    std::string name;
    std::string type;
    std::optional<std::string> constraint;
    SourceRange range;
};

struct EnumMemberDecl
{
    std::string name;
    std::optional<std::string> value;
    std::optional<std::string> value_kind;
    SourceRange range;
};

struct EnumDecl
{
    std::string name;
    std::vector<EnumMemberDecl> members;
    SourceRange range;
};

struct EventDecl
{
    std::string name;
    std::vector<FieldDecl> fields;
    SourceRange range;
};

struct FeatureFlagDecl
{
    std::string name;
    std::optional<std::string> type;
    std::optional<std::string> default_value;
    std::optional<std::string> default_value_kind;
    std::optional<std::string> scope;
    std::optional<std::string> owner;
    std::optional<std::string> description;
    std::optional<std::string> expires;
    SourceRange range;
};

struct ShapeDecl
{
    std::string name;
    std::vector<FieldDecl> fields;
    SourceRange range;
};

} // namespace statespec
