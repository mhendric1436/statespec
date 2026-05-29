#pragma once

#include "statespec/ir_core.hpp"

namespace statespec
{

struct IrValue
{
    std::string name;
    std::string type;
    std::optional<std::string> constraint;
};

struct IrEnumMember
{
    std::string name;
    std::optional<std::string> value;
};

struct IrEnum
{
    std::string name;
    std::vector<IrEnumMember> members;
};

struct IrEvent
{
    std::string name;
    std::vector<IrField> fields;
};

enum class IrFeatureFlagType
{
    Bool,
    String,
    Integer,
    Decimal,
};

enum class IrFeatureFlagScopeKind
{
    Tenant,
    System,
    User,
    Entity,
};

struct IrFeatureFlag
{
    std::string name;
    std::string type;
    IrFeatureFlagType flag_type{IrFeatureFlagType::Bool};
    std::string default_value;
    std::string scope;
    IrFeatureFlagScopeKind scope_kind{IrFeatureFlagScopeKind::Tenant};
    std::optional<std::string> owner;
    std::optional<std::string> description;
    std::optional<std::string> expires;
};

struct IrShape
{
    std::string name;
    std::vector<IrField> fields;
};

} // namespace statespec
