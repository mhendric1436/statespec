#pragma once

#include "backend.hpp"
#include "codec.hpp"

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
        ensure_collections(backend);
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
        memory_tx.put(
            kDefinitionsCollection, definition.name,
            detail::feature_flag_definition_to_json(definition)
        );
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
        auto record = as_memory_tx(tx).get(kDefinitionsCollection, name);
        if (!record.has_value())
        {
            return std::nullopt;
        }
        return detail::feature_flag_definition_from_json(record->document);
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
        if (const auto record = memory_tx.get(kValuesCollection, evaluation_key(request));
            record.has_value())
        {
            return detail::feature_flag_value_from_json(record->document);
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
        as_memory_tx(tx).put(
            kValuesCollection, evaluation_key(request), detail::feature_flag_value_to_json(value)
        );
    }

  private:
    static constexpr const char* kDefinitionsCollection = "statespec_feature_flag_definitions";
    static constexpr const char* kValuesCollection = "statespec_feature_flag_values";

    static void ensure_collections(IBackend& backend)
    {
        backend.ensure_collections(
            {CollectionDescriptor{.name = kDefinitionsCollection, .key_fields = {"name"}},
             CollectionDescriptor{.name = kValuesCollection, .key_fields = {"scope_key"}}}
        );
    }

    static std::string evaluation_key(const FeatureFlagEvaluationRequest& request)
    {
        return request.name + "|tenant:" + request.context.tenant_id.value_or("") +
               "|user:" + request.context.user_id.value_or("") +
               "|entity:" + request.context.entity_type.value_or("") + ":" +
               request.context.entity_id.value_or("");
    }
};

} // namespace statespec::backend::memory
