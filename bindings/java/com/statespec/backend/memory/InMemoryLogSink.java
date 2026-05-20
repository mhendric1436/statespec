package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.Log;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public final class InMemoryLogSink implements Log
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
        memoryTx.logDefinitionPuts.put(definition.name(), definition);
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
        if (memoryTx.logDefinitionPuts.containsKey(name))
        {
            return Optional.of(memoryTx.logDefinitionPuts.get(name));
        }
        synchronized (memoryTx.state)
        {
            return Optional.ofNullable(memoryTx.state.logDefinitions.get(name));
        }
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
        var memoryTx = InMemoryTransaction.require(tx);
        memoryTx.logEventAppends.add(event);
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
        var memoryTx = InMemoryTransaction.require(tx);
        var events = new ArrayList<Log.Event>();
        synchronized (memoryTx.state)
        {
            events.addAll(memoryTx.state.logEvents);
        }
        events.addAll(memoryTx.logEventAppends);
        return events;
    }
}
