package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.Json;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

public final class InMemoryTransaction implements Backend.Transaction
{
    static final class State
    {
        final Map<String, Map<String, Backend.VersionedRecord>> records = new HashMap<>();
        final Map<String, Long> versions = new HashMap<>();
        final Map<String, Backend.CollectionDescriptor> collections = new HashMap<>();
        final Map<String, Map<String, IndexState>> indexes = new HashMap<>();
    }

    static final class IndexState
    {
        final Backend.IndexDescriptor descriptor;
        final Map<List<String>, Set<String>> entries = new HashMap<>();

        IndexState(Backend.IndexDescriptor descriptor)
        {
            this.descriptor = descriptor;
        }
    }

    static Map<
        String,
        IndexState>
    emptyIndexStates(Backend.CollectionDescriptor descriptor)
    {
        var states = new HashMap<String, IndexState>();
        for (var index : descriptor.indexes())
        {
            states.put(index.name(), new IndexState(index));
        }
        return states;
    }

    static List<String> extractIndexKey(
        Json document,
        Backend.IndexDescriptor descriptor
    ) throws Backend.BackendException
    {
        var key = new ArrayList<String>();
        for (var field : descriptor.fields())
        {
            key.add(encodeIndexValue(document.find(field)));
        }
        return List.copyOf(key);
    }

    static String encodeIndexValue(Optional<Json> value) throws Backend.BackendException
    {
        if (value.isEmpty())
        {
            return "missing:";
        }
        return switch (value.orElseThrow())
        {
            case Json.NullValue ignored -> "null:";
            case Json.BooleanValue bool -> "bool:" + bool.value();
            case Json.IntegerValue integer -> "int:" + integer.value();
            case Json.DecimalValue decimal ->
                "decimal:" + new Json.DecimalValue(decimal.value()).canonicalString();
            case Json.StringValue string -> "string:" + string.value();
            case Json.ArrayValue ignored ->
                throw new Backend.BackendException(
                    "in-memory index fields must resolve to scalar JSON values"
                );
            case Json.ObjectValue ignored ->
                throw new Backend.BackendException(
                    "in-memory index fields must resolve to scalar JSON values"
                );
        };
    }

    static Map<
        String,
        Map<String,
            IndexState>>
    cloneIndexStates(
        Map<String,
            Map<String,
                IndexState>> indexes
    )
    {
        var clone = new HashMap<String, Map<String, IndexState>>();
        for (var collectionEntry : indexes.entrySet())
        {
            var collectionIndexes = new HashMap<String, IndexState>();
            for (var indexEntry : collectionEntry.getValue().entrySet())
            {
                var source = indexEntry.getValue();
                var copied = new IndexState(source.descriptor);
                for (var entry : source.entries.entrySet())
                {
                    copied.entries.put(entry.getKey(), new HashSet<>(entry.getValue()));
                }
                collectionIndexes.put(indexEntry.getKey(), copied);
            }
            clone.put(collectionEntry.getKey(), collectionIndexes);
        }
        return clone;
    }

    static void removeRecordFromIndexes(
        Map<String,
            Map<String,
                IndexState>> indexes,
        Backend.VersionedRecord record
    ) throws Backend.BackendException
    {
        var collectionIndexes = indexes.get(record.collection());
        if (collectionIndexes == null)
        {
            return;
        }
        for (var index : collectionIndexes.values())
        {
            var key = extractIndexKey(record.document(), index.descriptor);
            var entries = index.entries.get(key);
            if (entries == null)
            {
                continue;
            }
            entries.remove(record.key());
            if (entries.isEmpty())
            {
                index.entries.remove(key);
            }
        }
    }

    static void addRecordToIndexes(
        Map<String,
            Map<String,
                IndexState>> indexes,
        Backend.VersionedRecord record
    ) throws Backend.BackendException
    {
        var collectionIndexes = indexes.get(record.collection());
        if (collectionIndexes == null)
        {
            return;
        }
        for (var index : collectionIndexes.values())
        {
            var key = extractIndexKey(record.document(), index.descriptor);
            var entries = index.entries.computeIfAbsent(key, ignored -> new HashSet<>());
            if (index.descriptor.unique())
            {
                for (var existingKey : entries)
                {
                    if (!existingKey.equals(record.key()))
                    {
                        throw new Backend.ConflictException(
                            Backend.ConflictKind.UNIQUE_INDEX_CONFLICT,
                            "unique index conflict on collection '" + record.collection() +
                                "' index '" + index.descriptor.name() + "'"
                        );
                    }
                }
            }
            entries.add(record.key());
        }
    }

    final State state;
    boolean open = true;
    final Map<String, Long> readVersions = new HashMap<>();
    final Map<String, Backend.VersionedRecord> puts = new HashMap<>();
    final Set<String> erases = new HashSet<>();

    InMemoryTransaction(State state)
    {
        this.state = state;
    }

    @Override public boolean isOpen()
    {
        return open;
    }

    @Override public void abort()
    {
        open = false;
        readVersions.clear();
        puts.clear();
        erases.clear();
    }

    @Override
    public Optional<Backend.VersionedRecord>
    get(String collection,
        String key) throws Backend.BackendException
    {
        requireOpen();
        var versionKey = recordVersionKey(collection, key);
        if (erases.contains(versionKey))
        {
            return Optional.empty();
        }
        if (puts.containsKey(versionKey))
        {
            return Optional.of(puts.get(versionKey));
        }

        synchronized (state)
        {
            var collectionRecords = state.records.get(collection);
            if (collectionRecords == null || !collectionRecords.containsKey(key))
            {
                readVersions.put(versionKey, 0L);
                return Optional.empty();
            }
            var record = collectionRecords.get(key);
            readVersions.put(versionKey, record.version());
            return Optional.of(record);
        }
    }

    @Override
    public List<Backend.VersionedRecord> query(
        String collection,
        Backend.Query query
    ) throws Backend.BackendException
    {
        requireOpen();
        var byKey = new HashMap<String, Backend.VersionedRecord>();
        synchronized (state)
        {
            var collectionRecords = state.records.get(collection);
            if (collectionRecords != null)
            {
                byKey.putAll(collectionRecords);
            }
        }
        for (var record : puts.values())
        {
            if (record.collection().equals(collection))
            {
                byKey.put(record.key(), record);
            }
        }
        for (var versionKey : erases)
        {
            var prefix = "record:" + collection + ":";
            if (versionKey.startsWith(prefix))
            {
                byKey.remove(versionKey.substring(prefix.length()));
            }
        }

        var matched = new ArrayList<Backend.VersionedRecord>();
        for (var record : byKey.values())
        {
            if (matches(record, query))
            {
                readVersions.put(
                    recordVersionKey(record.collection(), record.key()), record.version()
                );
                matched.add(record);
            }
        }
        return matched;
    }

    @Override
    public void
    put(String collection,
        String key,
        Json document) throws Backend.BackendException
    {
        requireOpen();
        get(collection, key);
        var versionKey = recordVersionKey(collection, key);
        puts.put(versionKey, new Backend.VersionedRecord(collection, key, 0L, document));
        erases.remove(versionKey);
    }

    @Override
    public void erase(
        String collection,
        String key
    ) throws Backend.BackendException
    {
        requireOpen();
        get(collection, key);
        var versionKey = recordVersionKey(collection, key);
        puts.remove(versionKey);
        erases.add(versionKey);
    }

    void commit() throws Backend.BackendException
    {
        requireOpen();
        synchronized (state)
        {
            for (var entry : readVersions.entrySet())
            {
                if (versionOrZero(state.versions, entry.getKey()) != entry.getValue())
                {
                    throw new Backend.ConflictException(
                        Backend.ConflictKind.VERSION_CONFLICT, "in-memory OCC conflict"
                    );
                }
            }

            var stagedIndexes = cloneIndexStates(state.indexes);
            for (var versionKey : erases)
            {
                var parsed = parseRecordVersionKey(versionKey);
                if (parsed.isEmpty())
                {
                    continue;
                }
                var collectionRecords = state.records.get(parsed.get().collection());
                if (collectionRecords != null)
                {
                    var record = collectionRecords.get(parsed.get().key());
                    if (record != null)
                    {
                        removeRecordFromIndexes(stagedIndexes, record);
                    }
                }
            }
            for (var record : puts.values())
            {
                var collectionRecords = state.records.get(record.collection());
                if (collectionRecords != null)
                {
                    var existing = collectionRecords.get(record.key());
                    if (existing != null)
                    {
                        removeRecordFromIndexes(stagedIndexes, existing);
                    }
                }
                addRecordToIndexes(stagedIndexes, record);
            }

            for (var versionKey : erases)
            {
                eraseCommittedRecord(versionKey);
                state.versions.put(versionKey, versionOrZero(state.versions, versionKey) + 1);
            }
            for (var entry : puts.entrySet())
            {
                var versionKey = entry.getKey();
                var version = versionOrZero(state.versions, versionKey) + 1;
                state.versions.put(versionKey, version);
                var record = entry.getValue();
                state.records.computeIfAbsent(record.collection(), ignored -> new HashMap<>())
                    .put(
                        record.key(),
                        new Backend.VersionedRecord(
                            record.collection(), record.key(), version, record.document()
                        )
                    );
            }
            state.indexes.clear();
            state.indexes.putAll(stagedIndexes);
        }
        abort();
    }

    static InMemoryTransaction require(Backend.Transaction tx) throws Backend.BackendException
    {
        if (!(tx instanceof InMemoryTransaction memoryTx) || !memoryTx.isOpen())
        {
            throw new Backend.BackendException("expected open in-memory transaction");
        }
        return memoryTx;
    }

    private void requireOpen() throws Backend.BackendException
    {
        if (!isOpen())
        {
            throw new Backend.BackendException("expected open in-memory transaction");
        }
    }

    static String recordVersionKey(
        String collection,
        String key
    )
    {
        return "record:" + collection + ":" + key;
    }

    record ParsedRecordKey(
        String collection,
        String key
    )
    {
    }

    static Optional<ParsedRecordKey> parseRecordVersionKey(String versionKey)
    {
        var prefix = "record:";
        if (!versionKey.startsWith(prefix))
        {
            return Optional.empty();
        }
        var rest = versionKey.substring(prefix.length());
        var separator = rest.indexOf(':');
        if (separator < 0)
        {
            return Optional.empty();
        }
        return Optional.of(
            new ParsedRecordKey(rest.substring(0, separator), rest.substring(separator + 1))
        );
    }

    static long versionOrZero(
        Map<String,
            Long> versions,
        String key
    )
    {
        return versions.getOrDefault(key, 0L);
    }

    static String definitionKey(Object... parts)
    {
        StringBuilder key = new StringBuilder();
        for (int index = 0; index < parts.length; index++)
        {
            if (index > 0)
            {
                key.append(':');
            }
            key.append(parts[index]);
        }
        return key.toString();
    }

    static Json findJsonPath(
        Json document,
        String path
    )
    {
        Json current = document;
        for (String segment : path.split("\\."))
        {
            var next = current.find(segment);
            if (next.isEmpty())
            {
                return null;
            }
            current = next.get();
        }
        return current;
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
        Backend.VersionedRecord record,
        Backend.Query query
    )
    {
        if (query instanceof Backend.Query.All)
        {
            return true;
        }
        if (query instanceof Backend.Query.KeyPrefix keyPrefix)
        {
            return record.key().startsWith(keyPrefix.prefix());
        }
        if (query instanceof Backend.Query.JsonEquals jsonEquals)
        {
            var value = findJsonPath(record.document(), jsonEquals.path());
            return value != null &&
                value.canonicalString().equals(jsonEquals.value().canonicalString());
        }
        return true;
    }
}
