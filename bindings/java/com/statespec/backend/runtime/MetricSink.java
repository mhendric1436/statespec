package com.statespec.backend.runtime;

import com.statespec.backend.Backend;
import com.statespec.backend.Metric;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;

public final class MetricSink implements Metric
{
    private static final String DEFINITIONS = "metrics.definitions";
    private static final String SAMPLES = "metrics.samples";

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
        var existing = inspectDefinitionTx(tx, definition.name());
        tx.put(
            DEFINITIONS, definition.name(), ObservabilityCodec.metricDefinitionToJson(definition)
        );
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
        return tx.get(DEFINITIONS, name)
            .map(record -> ObservabilityCodec.metricDefinitionFromJson(record.document()));
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
        var samples = tx.query(SAMPLES, new Backend.Query.All());
        tx.put(SAMPLES, sampleKey(samples.size()), ObservabilityCodec.metricSampleToJson(sample));
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
        var samples = new ArrayList<Metric.Sample>();
        var records = tx.query(SAMPLES, new Backend.Query.All());
        records.sort(Comparator.comparing(Backend.VersionedRecord::key));
        for (var record : records)
        {
            samples.add(ObservabilityCodec.metricSampleFromJson(record.document()));
        }
        return samples;
    }

    private static String sampleKey(int index)
    {
        return String.format("sample:%020d", index + 1);
    }
}
