package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.FeatureFlag;
import java.util.Optional;

public final class InMemoryFeatureFlagStore implements FeatureFlag
{
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
        memoryTx.featureFlagDefinitionPuts.put(definition.name(), definition);
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
        if (memoryTx.featureFlagDefinitionPuts.containsKey(name))
        {
            return Optional.of(memoryTx.featureFlagDefinitionPuts.get(name));
        }
        synchronized (memoryTx.state)
        {
            return Optional.ofNullable(memoryTx.state.featureFlagDefinitions.get(name));
        }
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
        if (memoryTx.featureFlagValuePuts.containsKey(request.name()))
        {
            return memoryTx.featureFlagValuePuts.get(request.name());
        }
        synchronized (memoryTx.state)
        {
            if (memoryTx.state.featureFlagValues.containsKey(request.name()))
            {
                return memoryTx.state.featureFlagValues.get(request.name());
            }
        }
        var definition = inspectDefinitionTx(tx, request.name());
        if (definition.isEmpty())
        {
            throw new Backend.BackendException("unknown feature flag " + request.name());
        }
        return definition.get().defaultValue();
    }
}
