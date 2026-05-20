package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.Json;
import java.util.ArrayList;
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
        var versionKey = InMemoryTransaction.recordVersionKey(collection, key);
        if (memoryTx.erases.contains(versionKey))
        {
            return Optional.empty();
        }
        if (memoryTx.puts.containsKey(versionKey))
        {
            return Optional.of(memoryTx.puts.get(versionKey));
        }

        synchronized (state)
        {
            var collectionRecords = state.records.get(collection);
            if (collectionRecords == null || !collectionRecords.containsKey(key))
            {
                memoryTx.readVersions.put(versionKey, 0L);
                return Optional.empty();
            }
            var record = collectionRecords.get(key);
            memoryTx.readVersions.put(versionKey, record.version());
            return Optional.of(record);
        }
    }

    @Override
    public List<VersionedRecord> query(
        Transaction tx,
        String collection,
        Query query
    ) throws BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        var byKey = new java.util.HashMap<String, VersionedRecord>();
        synchronized (state)
        {
            var collectionRecords = state.records.get(collection);
            if (collectionRecords != null)
            {
                byKey.putAll(collectionRecords);
            }
        }
        for (var record : memoryTx.puts.values())
        {
            if (record.collection().equals(collection))
            {
                byKey.put(record.key(), record);
            }
        }
        for (var versionKey : memoryTx.erases)
        {
            var prefix = "record:" + collection + ":";
            if (versionKey.startsWith(prefix))
            {
                byKey.remove(versionKey.substring(prefix.length()));
            }
        }

        var matched = new ArrayList<VersionedRecord>();
        for (var record : byKey.values())
        {
            if (matches(record, query))
            {
                memoryTx.readVersions.put(
                    InMemoryTransaction.recordVersionKey(record.collection(), record.key()),
                    record.version()
                );
                matched.add(record);
            }
        }
        return matched;
    }

    @Override
    public void
    put(Transaction tx,
        String collection,
        String key,
        Json document) throws BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        get(tx, collection, key);
        var versionKey = InMemoryTransaction.recordVersionKey(collection, key);
        memoryTx.puts.put(versionKey, new VersionedRecord(collection, key, 0L, document));
        memoryTx.erases.remove(versionKey);
    }

    @Override
    public void erase(
        Transaction tx,
        String collection,
        String key
    ) throws BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        get(tx, collection, key);
        var versionKey = InMemoryTransaction.recordVersionKey(collection, key);
        memoryTx.puts.remove(versionKey);
        memoryTx.erases.add(versionKey);
    }

    @Override public void commit(Transaction tx) throws BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        synchronized (state)
        {
            for (var entry : memoryTx.readVersions.entrySet())
            {
                if (InMemoryTransaction.versionOrZero(state.versions, entry.getKey()) !=
                    entry.getValue())
                {
                    throw new ConflictException(
                        ConflictKind.VERSION_CONFLICT, "in-memory OCC conflict"
                    );
                }
            }
            for (var versionKey : memoryTx.erases)
            {
                eraseCommittedRecord(versionKey);
                state.versions.put(
                    versionKey, InMemoryTransaction.versionOrZero(state.versions, versionKey) + 1
                );
            }
            for (var entry : memoryTx.puts.entrySet())
            {
                var versionKey = entry.getKey();
                var version = InMemoryTransaction.versionOrZero(state.versions, versionKey) + 1;
                state.versions.put(versionKey, version);
                var record = entry.getValue();
                state.records
                    .computeIfAbsent(record.collection(), ignored -> new java.util.HashMap<>())
                    .put(
                        record.key(),
                        new VersionedRecord(
                            record.collection(), record.key(), version, record.document()
                        )
                    );
            }
            state.featureFlagDefinitions.putAll(memoryTx.featureFlagDefinitionPuts);
            state.featureFlagValues.putAll(memoryTx.featureFlagValuePuts);
            state.queueDefinitions.putAll(memoryTx.queueDefinitionPuts);
            state.queueMessages.putAll(memoryTx.queueMessagePuts);
            state.queueIdempotencyKeys.putAll(memoryTx.queueIdempotencyPuts);
            state.leaseDefinitions.putAll(memoryTx.leaseDefinitionPuts);
            for (var key : memoryTx.leaseErases)
            {
                state.leases.remove(key);
            }
            state.leases.putAll(memoryTx.leasePuts);
            state.workflowDefinitions.putAll(memoryTx.workflowDefinitionPuts);
            state.workflowExecutions.putAll(memoryTx.workflowExecutionPuts);
            state.logDefinitions.putAll(memoryTx.logDefinitionPuts);
            state.logEvents.addAll(memoryTx.logEventAppends);
            state.metricDefinitions.putAll(memoryTx.metricDefinitionPuts);
            state.metricSamples.addAll(memoryTx.metricSampleAppends);
        }
        memoryTx.abort();
    }

    private void eraseCommittedRecord(String versionKey)
    {
        var prefix = "record:";
        if (!versionKey.startsWith(prefix))
        {
            return;
        }
        var rest = versionKey.substring(prefix.length());
        var separator = rest.indexOf(':');
        if (separator < 0)
        {
            return;
        }
        var collection = rest.substring(0, separator);
        var key = rest.substring(separator + 1);
        var records = state.records.get(collection);
        if (records != null)
        {
            records.remove(key);
        }
    }

    private static boolean matches(
        VersionedRecord record,
        Query query
    )
    {
        if (query instanceof Query.All)
        {
            return true;
        }
        if (query instanceof Query.KeyPrefix keyPrefix)
        {
            return record.key().startsWith(keyPrefix.prefix());
        }
        if (query instanceof Query.JsonEquals jsonEquals)
        {
            var value = InMemoryTransaction.findJsonPath(record.document(), jsonEquals.path());
            return value != null &&
                value.canonicalString().equals(jsonEquals.value().canonicalString());
        }
        return true;
    }
}
