package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.Metric;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public final class InMemoryMetricSink implements Metric
{
    @Override
    public DefinitionRegistration registerDefinition(
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
    public DefinitionRegistration registerDefinitionTx(
        Backend.Transaction tx,
        Definition definition
    ) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        var existing = inspectDefinitionTx(tx, definition.name());
        memoryTx.metricDefinitionPuts.put(definition.name(), definition);
        return new DefinitionRegistration(existing.isEmpty(), definition);
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
        if (memoryTx.metricDefinitionPuts.containsKey(name))
        {
            return Optional.of(memoryTx.metricDefinitionPuts.get(name));
        }
        synchronized (memoryTx.state)
        {
            return Optional.ofNullable(memoryTx.state.metricDefinitions.get(name));
        }
    }

    @Override
    public void record(
        Backend backend,
        Sample sample
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            recordTx(tx, sample);
            backend.commit(tx);
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public void recordTx(
        Backend.Transaction tx,
        Sample sample
    ) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        memoryTx.metricSampleAppends.add(sample);
    }

    public List<Sample> inspectSamples(Backend backend) throws Backend.BackendException
    {
        var tx = backend.begin();
        var samples = inspectSamplesTx(tx);
        backend.commit(tx);
        return samples;
    }

    public List<Sample> inspectSamplesTx(Backend.Transaction tx) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        var samples = new ArrayList<Metric.Sample>();
        synchronized (memoryTx.state)
        {
            samples.addAll(memoryTx.state.metricSamples);
        }
        samples.addAll(memoryTx.metricSampleAppends);
        return samples;
    }
}
