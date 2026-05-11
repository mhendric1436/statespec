package com.statespec.backend;

import java.util.List;
import java.util.Optional;

public final class BackendModel {
    private BackendModel() {}

    public record FieldDescriptor(
        String name,
        String type,
        boolean required
    ) {}

    public record IndexDescriptor(
        String name,
        List<String> fields,
        boolean unique
    ) {}

    public record CollectionDescriptor(
        String name,
        List<FieldDescriptor> fields,
        List<String> keyFields,
        List<IndexDescriptor> indexes,
        long schemaVersion
    ) {}

    public record BackendCapabilities(
        boolean transactions,
        boolean compareAndSwap,
        boolean prefixQuery,
        boolean secondaryIndexes,
        boolean uniqueIndexes,
        boolean jsonPathQuery,
        boolean orderedScan,
        boolean durableHistory,
        boolean schemaSnapshots
    ) {}

    public enum ConflictKind {
        VERSION_CONFLICT,
        PREDICATE_CONFLICT,
        UNIQUE_INDEX_CONFLICT,
        SCHEMA_CONFLICT,
        LEASE_CONFLICT,
        QUEUE_CLAIM_CONFLICT,
        WORKFLOW_CLAIM_CONFLICT
    }

    public static class BackendException extends Exception {
        public BackendException(String message) {
            super(message);
        }

        public BackendException(String message, Throwable cause) {
            super(message, cause);
        }
    }

    public static class ConflictException extends BackendException {
        private final ConflictKind kind;

        public ConflictException(ConflictKind kind, String message) {
            super(message);
            this.kind = kind;
        }

        public ConflictKind kind() {
            return kind;
        }
    }

    public record VersionedRecord(
        String collection,
        String key,
        long version,
        String documentJson
    ) {}

    public sealed interface Query
        permits Query.All, Query.KeyPrefix, Query.JsonEquals {

        record All() implements Query {}

        record KeyPrefix(String prefix) implements Query {}

        record JsonEquals(String path, String valueJson) implements Query {}
    }

    public interface Transaction {
        boolean isOpen();

        void abort() throws BackendException;
    }

    public interface Backend {
        BackendCapabilities capabilities();

        void ensureCollection(CollectionDescriptor descriptor) throws BackendException;

        void ensureCollections(List<CollectionDescriptor> descriptors) throws BackendException;

        Transaction begin() throws BackendException;

        Optional<VersionedRecord> get(
            Transaction tx,
            String collection,
            String key
        ) throws BackendException;

        List<VersionedRecord> query(
            Transaction tx,
            String collection,
            Query query
        ) throws BackendException;

        void put(
            Transaction tx,
            String collection,
            String key,
            String documentJson
        ) throws BackendException;

        void erase(
            Transaction tx,
            String collection,
            String key
        ) throws BackendException;

        void commit(Transaction tx) throws BackendException;
    }
}
