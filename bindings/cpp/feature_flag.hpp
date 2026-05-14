#pragma once

#include "backend.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace statespec::backend
{

enum class FeatureFlagType
{
    Bool,
    String,
    Integer,
    Decimal,
};

enum class FeatureFlagScopeKind
{
    System,
    Tenant,
    User,
    Entity,
};

class FeatureFlagValue
{
  public:
    using Storage = std::variant<bool, std::string, std::int64_t, double>;

    static FeatureFlagValue bool_value(bool value)
    {
        return FeatureFlagValue{FeatureFlagType::Bool, Storage{value}};
    }

    static FeatureFlagValue string_value(std::string value)
    {
        return FeatureFlagValue{FeatureFlagType::String, Storage{std::move(value)}};
    }

    static FeatureFlagValue integer_value(std::int64_t value)
    {
        return FeatureFlagValue{FeatureFlagType::Integer, Storage{value}};
    }

    static FeatureFlagValue decimal_value(double value)
    {
        return FeatureFlagValue{FeatureFlagType::Decimal, Storage{value}};
    }

    FeatureFlagType type() const noexcept
    {
        return type_;
    }

    std::optional<bool> as_bool() const
    {
        if (const auto* value = std::get_if<bool>(&value_))
        {
            return *value;
        }
        return std::nullopt;
    }

    std::optional<std::string> as_string() const
    {
        if (const auto* value = std::get_if<std::string>(&value_))
        {
            return *value;
        }
        return std::nullopt;
    }

    std::optional<std::int64_t> as_integer() const
    {
        if (const auto* value = std::get_if<std::int64_t>(&value_))
        {
            return *value;
        }
        return std::nullopt;
    }

    std::optional<double> as_decimal() const
    {
        if (const auto* value = std::get_if<double>(&value_))
        {
            return *value;
        }
        return std::nullopt;
    }

  private:
    FeatureFlagValue(
        FeatureFlagType type,
        Storage value
    )
        : type_(type),
          value_(std::move(value))
    {
    }

    FeatureFlagType type_;
    Storage value_;
};

struct FeatureFlagDefinition
{
    std::string name;
    FeatureFlagType type;
    FeatureFlagValue default_value;
    FeatureFlagScopeKind scope;
    std::optional<std::string> owner;
    std::optional<std::string> description;
    std::optional<std::string> expires;
};

struct FeatureFlagRegisterDefinitionResult
{
    bool registered_new = false;
    FeatureFlagDefinition definition;
};

struct FeatureFlagEvaluationContext
{
    std::optional<std::string> tenant_id;
    std::optional<std::string> user_id;
    std::optional<std::string> entity_type;
    std::optional<std::string> entity_id;
};

struct FeatureFlagEvaluationRequest
{
    std::string name;
    FeatureFlagEvaluationContext context;
};

class IFeatureFlagStore
{
  public:
    virtual ~IFeatureFlagStore() = default;

    virtual FeatureFlagRegisterDefinitionResult register_definition(
        IBackend& backend,
        const FeatureFlagDefinition& definition
    ) = 0;

    virtual FeatureFlagRegisterDefinitionResult register_definitionTx(
        ITransaction& tx,
        const FeatureFlagDefinition& definition
    ) = 0;

    virtual std::optional<FeatureFlagDefinition> inspect_definition(
        IBackend& backend,
        const std::string& name
    ) = 0;

    virtual std::optional<FeatureFlagDefinition> inspect_definitionTx(
        ITransaction& tx,
        const std::string& name
    ) = 0;

    virtual FeatureFlagValue evaluate(
        IBackend& backend,
        const FeatureFlagEvaluationRequest& request
    ) = 0;

    virtual FeatureFlagValue evaluateTx(
        ITransaction& tx,
        const FeatureFlagEvaluationRequest& request
    ) = 0;
};

} // namespace statespec::backend
