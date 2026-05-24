package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.Json;
import com.statespec.backend.SchemaCompatibility;
import java.util.HashMap;
import java.util.List;
import java.util.Optional;

public final class InMemoryBackend implements Backend
{
    private final InMemoryTransaction.State state = new InMemoryTransaction.State();

    @Override public BackendCapabilities capabilities()
    {
        return new BackendCapabilities(true, true, true, true, true, false, false, false, false);
    }

    @Override public void ensureCollection(CollectionDescriptor descriptor) throws BackendException
    {
        synchronized (state)
        {
            var existing = state.collections.get(descriptor.name());
            if (existing != null)
            {
                SchemaCompatibility.validateCollectionDescriptorUpgrade(existing, descriptor);
                state.collections.put(descriptor.name(), descriptor);
                return;
            }
            state.collections.put(descriptor.name(), descriptor);
            state.indexes.put(descriptor.name(), InMemoryTransaction.emptyIndexStates(descriptor));
        }
    }

    @Override
    public void ensureCollections(List<CollectionDescriptor> descriptors) throws BackendException
    {
        synchronized (state)
        {
            var staged = new HashMap<>(state.collections);
            var stagedIndexes = new HashMap<>(state.indexes);
            for (var descriptor : descriptors)
            {
                var existing = staged.get(descriptor.name());
                if (existing != null)
                {
                    SchemaCompatibility.validateCollectionDescriptorUpgrade(existing, descriptor);
                }
                staged.put(descriptor.name(), descriptor);
                if (!state.collections.containsKey(descriptor.name()))
                {
                    stagedIndexes.put(
                        descriptor.name(), InMemoryTransaction.emptyIndexStates(descriptor)
                    );
                }
            }
            state.collections.clear();
            state.collections.putAll(staged);
            state.indexes.clear();
            state.indexes.putAll(stagedIndexes);
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
        return tx.get(collection, key);
    }

    @Override
    public List<VersionedRecord> query(
        Transaction tx,
        String collection,
        Query query
    ) throws BackendException
    {
        return tx.query(collection, query);
    }

    @Override
    public void
    put(Transaction tx,
        String collection,
        String key,
        Json document) throws BackendException
    {
        tx.put(collection, key, document);
    }

    @Override
    public void erase(
        Transaction tx,
        String collection,
        String key
    ) throws BackendException
    {
        tx.erase(collection, key);
    }

    @Override public void commit(Transaction tx) throws BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        memoryTx.commit();
    }
}
