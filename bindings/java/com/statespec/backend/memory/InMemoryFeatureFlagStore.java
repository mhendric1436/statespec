package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.FeatureFlag;
import java.util.Optional;

public final class InMemoryFeatureFlagStore implements FeatureFlag
{
    private static final String DEFINITIONS = "feature_flags.definitions";
    private static final String VALUES = "feature_flags.values";

    @Override
    public RegisterDefinitionResult registerDefinition(
        Backend backend,
        Definition definition
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = registerDefinitionTx(tx, definition);
            backend.commit(tx);
            return result;
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public RegisterDefinitionResult registerDefinitionTx(
        Backend.Transaction tx,
        Definition definition
    ) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        var existing = inspectDefinitionTx(tx, definition.name());
        memoryTx.put(
            DEFINITIONS, definition.name(), InMemoryCodec.featureFlagDefinitionToJson(definition)
        );
        return new RegisterDefinitionResult(existing.isEmpty(), definition);
    }

    @Override
    public Optional<Definition> inspectDefinition(
        Backend backend,
        String name
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        var result = inspectDefinitionTx(tx, name);
        backend.commit(tx);
        return result;
    }

    @Override
    public Optional<Definition> inspectDefinitionTx(
        Backend.Transaction tx,
        String name
    ) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        return memoryTx.get(DEFINITIONS, name)
            .map(record -> InMemoryCodec.featureFlagDefinitionFromJson(record.document()));
    }

    @Override
    public Value evaluate(
        Backend backend,
        EvaluationRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        var result = evaluateTx(tx, request);
        backend.commit(tx);
        return result;
    }

    @Override
    public Value evaluateTx(
        Backend.Transaction tx,
        EvaluationRequest request
    ) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        var override = memoryTx.get(VALUES, request.name());
        if (override.isPresent())
        {
            return InMemoryCodec.featureFlagValueFromJson(override.get().document());
        }
        var definition = inspectDefinitionTx(tx, request.name());
        if (definition.isEmpty())
        {
            throw new Backend.BackendException("unknown feature flag " + request.name());
        }
        return definition.get().defaultValue();
    }
}
