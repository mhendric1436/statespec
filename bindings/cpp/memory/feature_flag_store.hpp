#pragma once

#include "backend.hpp"

namespace statespec::backend::memory
{

class InMemoryFeatureFlagStore : public IFeatureFlagStore
{
  public:
    FeatureFlagRegisterDefinitionResult register_definition(
        IBackend& backend,
        const FeatureFlagDefinition& definition
    ) override
    {
        auto tx = backend.begin();
        auto result = register_definitionTx(*tx, definition);
        backend.commit(*tx);
        return result;
    }

    FeatureFlagRegisterDefinitionResult register_definitionTx(
        ITransaction& tx,
        const FeatureFlagDefinition& definition
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto existing = inspect_definitionTx(memory_tx, definition.name);
        memory_tx.feature_flag_definition_puts().insert_or_assign(definition.name, definition);
        return FeatureFlagRegisterDefinitionResult{!existing.has_value(), definition};
    }

    std::optional<FeatureFlagDefinition> inspect_definition(
        IBackend& backend,
        const std::string& name
    ) override
    {
        auto tx = backend.begin();
        auto result = inspect_definitionTx(*tx, name);
        backend.commit(*tx);
        return result;
    }

    std::optional<FeatureFlagDefinition> inspect_definitionTx(
        ITransaction& tx,
        const std::string& name
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        if (const auto staged = memory_tx.feature_flag_definition_puts().find(name);
            staged != memory_tx.feature_flag_definition_puts().end())
        {
            return staged->second;
        }
        std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
        if (const auto iter = memory_tx.state().feature_flag_definitions.find(name);
            iter != memory_tx.state().feature_flag_definitions.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    FeatureFlagValue evaluate(
        IBackend& backend,
        const FeatureFlagEvaluationRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = evaluateTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    FeatureFlagValue evaluateTx(
        ITransaction& tx,
        const FeatureFlagEvaluationRequest& request
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto value_key = evaluation_key(request);
        if (const auto staged = memory_tx.feature_flag_value_puts().find(value_key);
            staged != memory_tx.feature_flag_value_puts().end())
        {
            return staged->second;
        }
        {
            std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
            if (const auto iter = memory_tx.state().feature_flag_values.find(value_key);
                iter != memory_tx.state().feature_flag_values.end())
            {
                return iter->second;
            }
        }
        const auto definition = inspect_definitionTx(memory_tx, request.name);
        if (!definition.has_value())
        {
            throw BackendError("unknown feature flag: " + request.name);
        }
        return definition->default_value;
    }

    void set_overrideTx(
        ITransaction& tx,
        const FeatureFlagEvaluationRequest& request,
        FeatureFlagValue value
    )
    {
        auto& memory_tx = as_memory_tx(tx);
        memory_tx.feature_flag_value_puts().insert_or_assign(evaluation_key(request), value);
    }

  private:
    static std::string evaluation_key(const FeatureFlagEvaluationRequest& request)
    {
        return request.name + "|tenant:" + request.context.tenant_id.value_or("") +
               "|user:" + request.context.user_id.value_or("") +
               "|entity:" + request.context.entity_type.value_or("") + ":" +
               request.context.entity_id.value_or("");
    }
};

} // namespace statespec::backend::memory
