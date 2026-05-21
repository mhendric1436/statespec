#pragma once

#include "statespec/semantic_core.hpp"

namespace statespec
{

struct SemanticFeatureFlag
{
    std::string name;
    std::string type;
    std::string default_value;
    std::string scope;
    std::optional<SemanticReference> scope_target;
    std::optional<std::string> owner;
    std::optional<std::string> description;
    std::optional<std::string> expires;
};

struct SemanticValue
{
    std::string name;
    std::string type;
    std::optional<std::string> constraint;
};

struct SemanticEnumMember
{
    std::string name;
    std::optional<std::string> value;
};

struct SemanticEnum
{
    std::string name;
    std::vector<SemanticEnumMember> members;
};

struct SemanticEvent
{
    std::string name;
    std::vector<SemanticField> fields;
};

struct SemanticShape
{
    std::string name;
    std::vector<SemanticField> fields;
};

} // namespace statespec
