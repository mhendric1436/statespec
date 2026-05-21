package com.statespec.backend;

import java.math.BigDecimal;
import java.util.List;
import java.util.Optional;

public interface Backend
{
    enum FieldType
    {
        STRING,
        BOOL,
        INT,
        INT32,
        INT64,
        LONG,
        DOUBLE,
        DECIMAL,
        JSON,
        TIMESTAMP,
        DURATION,
        UUID,
        NAMED,
        LIST,
        SET,
        MAP,
        OPTIONAL,
        REFERENCE
    }

    record FieldDescriptor(
        String name,
        FieldType type,
        String typeName,
        boolean required
    )
    {
    }

    record IndexDescriptor(
        String name,
        List<String> fields,
        boolean unique
    )
    {
    }

    record CollectionDescriptor(
        String name,
        List<FieldDescriptor> fields,
        List<String> keyFields,
        List<IndexDescriptor> indexes,
        long schemaVersion
    )
    {
    }

    record BackendCapabilities(
        boolean transactions,
        boolean compareAndSwap,
        boolean prefixQuery,
        boolean secondaryIndexes,
        boolean uniqueIndexes,
        boolean jsonPathQuery,
        boolean orderedScan,
        boolean durableHistory,
        boolean schemaSnapshots
    )
    {
    }

    enum ConflictKind
    {
        VERSION_CONFLICT,
        PREDICATE_CONFLICT,
        UNIQUE_INDEX_CONFLICT,
        SCHEMA_CONFLICT,
        LEASE_CONFLICT,
        QUEUE_CLAIM_CONFLICT,
        WORKFLOW_CLAIM_CONFLICT
    }

    class BackendException extends Exception
    {
        public BackendException(String message)
        {
            super(message);
        }

        public BackendException(
            String message,
            Throwable cause
        )
        {
            super(message, cause);
        }
    }

    class ConflictException extends BackendException
    {
        private final ConflictKind kind;

        public ConflictException(
            ConflictKind kind,
            String message
        )
        {
            super(message);
            this.kind = kind;
        }

        public ConflictKind kind()
        {
            return kind;
        }
    }

    record VersionedRecord(
        String collection,
        String key,
        long version,
        Json document
    )
    {
    }

    sealed interface IndexValue permits IndexValue.NullValue, IndexValue.StringValue,
        IndexValue.IntegerValue, IndexValue.DecimalValue, IndexValue.BooleanValue,
        IndexValue.TimestampValue {

        Json value();

        record NullValue() implements IndexValue
        {
            @Override public Json value()
            {
                return Json.nullValue();
            }
        }

        record StringValue(String raw) implements IndexValue
        {
            @Override public Json value()
            {
                return Json.string(raw);
            }
        }

        record IntegerValue(long raw) implements IndexValue
        {
            @Override public Json value()
            {
                return Json.integer(raw);
            }
        }

        record DecimalValue(BigDecimal raw) implements IndexValue
        {
            public DecimalValue(String raw)
            {
                this(new BigDecimal(raw));
            }

            @Override public Json value()
            {
                return Json.decimal(raw);
            }
        }

        record BooleanValue(boolean raw) implements IndexValue
        {
            @Override public Json value()
            {
                return Json.bool(raw);
            }
        }

        record TimestampValue(String raw) implements IndexValue
        {
            @Override public Json value()
            {
                return Json.string(raw);
            }
        }
    }

    record IndexBound(
        List<IndexValue> values,
        boolean inclusive
    )
    {
    }

    sealed interface Query permits Query.All, Query.KeyPrefix, Query.JsonEquals, Query.IndexEquals,
        Query.IndexPrefix, Query.IndexRange {

        record All() implements Query
        {
        }

        record KeyPrefix(String prefix) implements Query
        {
        }

        record JsonEquals(
            String path,
            Json value
        ) implements Query
        {
        }

        record IndexEquals(
            String indexName,
            List<IndexValue> values
        ) implements Query
        {
        }

        record IndexPrefix(
            String indexName,
            List<IndexValue> prefixValues
        ) implements Query
        {
        }

        record IndexRange(
            String indexName,
            Optional<IndexBound> lowerBound,
            Optional<IndexBound> upperBound
        ) implements Query
        {
        }
    }

    interface Transaction
    {
        boolean isOpen();

        void abort() throws BackendException;

        Optional<VersionedRecord>
        get(String collection,
            String key) throws BackendException;

        List<VersionedRecord> query(
            String collection,
            Query query
        ) throws BackendException;

        void
        put(String collection,
            String key,
            Json document) throws BackendException;

        void erase(
            String collection,
            String key
        ) throws BackendException;
    }

    BackendCapabilities capabilities();

    void ensureCollection(CollectionDescriptor descriptor) throws BackendException;

    void ensureCollections(List<CollectionDescriptor> descriptors) throws BackendException;

    Transaction begin() throws BackendException;

    Optional<VersionedRecord>
    get(Transaction tx,
        String collection,
        String key) throws BackendException;

    List<VersionedRecord> query(
        Transaction tx,
        String collection,
        Query query
    ) throws BackendException;

    void
    put(Transaction tx,
        String collection,
        String key,
        Json document) throws BackendException;

    void erase(
        Transaction tx,
        String collection,
        String key
    ) throws BackendException;

    void commit(Transaction tx) throws BackendException;
}
