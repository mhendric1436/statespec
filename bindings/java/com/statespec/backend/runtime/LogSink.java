package com.statespec.backend.runtime;

import com.statespec.backend.Backend;
import com.statespec.backend.Log;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;

public final class LogSink implements Log
{
    private static final String DEFINITIONS = "logs.definitions";
    private static final String EVENTS = "logs.events";

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
        tx.put(DEFINITIONS, definition.name(), LogCodec.logDefinitionToJson(definition));
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
            .map(record -> LogCodec.logDefinitionFromJson(record.document()));
    }

    @Override
    public void emit(
        Backend backend,
        Event event
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            emitTx(tx, event);
            backend.commit(tx);
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public void emitTx(
        Backend.Transaction tx,
        Event event
    ) throws Backend.BackendException
    {
        var events = tx.query(EVENTS, new Backend.Query.All());
        tx.put(EVENTS, eventKey(events.size()), LogCodec.logEventToJson(event));
    }

    public List<Event> inspectEvents(Backend backend) throws Backend.BackendException
    {
        var tx = backend.begin();
        var events = inspectEventsTx(tx);
        backend.commit(tx);
        return events;
    }

    public List<Event> inspectEventsTx(Backend.Transaction tx) throws Backend.BackendException
    {
        var events = new ArrayList<Log.Event>();
        var records = tx.query(EVENTS, new Backend.Query.All());
        records.sort(Comparator.comparing(Backend.VersionedRecord::key));
        for (var record : records)
        {
            events.add(LogCodec.logEventFromJson(record.document()));
        }
        return events;
    }

    private static String eventKey(int index)
    {
        return String.format("event:%020d", index + 1);
    }
}
