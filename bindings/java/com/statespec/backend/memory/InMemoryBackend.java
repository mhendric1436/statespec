package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.Json;
import java.util.List;
import java.util.Optional;

public final class InMemoryBackend implements Backend
{
    private final InMemoryTransaction.State state = new InMemoryTransaction.State();

    @Override public BackendCapabilities capabilities()
    {
        return new BackendCapabilities(true, true, true, false, false, false, false, false, false);
    }

    @Override public void ensureCollection(CollectionDescriptor descriptor)
    {
        synchronized (state)
        {
            state.collections.put(descriptor.name(), descriptor);
        }
    }

    @Override public void ensureCollections(List<CollectionDescriptor> descriptors)
    {
        for (var descriptor : descriptors)
        {
            ensureCollection(descriptor);
        }
    }

    @Override public Transaction begin()
    {
        return new InMemoryTransaction(state);
    }

    @Override
    public Optional<VersionedRecord>
    get(Transaction tx,
        String collection,
        String key) throws BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        return memoryTx.get(collection, key);
    }

    @Override
    public List<VersionedRecord> query(
        Transaction tx,
        String collection,
        Query query
    ) throws BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        return memoryTx.query(collection, query);
    }

    @Override
    public void
    put(Transaction tx,
        String collection,
        String key,
        Json document) throws BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        memoryTx.put(collection, key, document);
    }

    @Override
    public void erase(
        Transaction tx,
        String collection,
        String key
    ) throws BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        memoryTx.erase(collection, key);
    }

    @Override public void commit(Transaction tx) throws BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        memoryTx.commit();
    }
}
