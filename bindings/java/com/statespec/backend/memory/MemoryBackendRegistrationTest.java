package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.Json;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public final class MemoryBackendRegistrationTest
{
    public static void main(String[] args) throws Backend.BackendException
    {
        enforcesCompatibleCollectionRegistration();
        appliesCollectionBatchesAtomically();
        extractsIndexKeysDeterministically();
    }

    private static void enforcesCompatibleCollectionRegistration() throws Backend.BackendException
    {
        var backend = new InMemoryBackend();
        var base = baseDescriptor();

        backend.ensureCollection(base);
        backend.ensureCollection(base);
        backend.ensureCollection(compatibleDescriptor());

        var incompatible = descriptor(
            compatibleDescriptor().fields(), List.of("tenant_id", "order_id"),
            compatibleDescriptor().indexes(), 3L
        );
        try
        {
            backend.ensureCollection(incompatible);
            throw new AssertionError("expected schema conflict");
        }
        catch (Backend.ConflictException error)
        {
            require(
                error.kind() == Backend.ConflictKind.SCHEMA_CONFLICT, "expected schema conflict"
            );
            require(error.getMessage().contains("orders"), "expected collection name in message");
            require(
                error.getMessage().contains("KEY_FIELDS_CHANGED"),
                "expected incompatibility reason in message"
            );
        }
    }

    private static void appliesCollectionBatchesAtomically() throws Backend.BackendException
    {
        var backend = new InMemoryBackend();
        backend.ensureCollection(baseDescriptor());

        var incompatible = descriptor(
            baseDescriptor().fields(), List.of("tenant_id", "order_id"), baseDescriptor().indexes(),
            2L
        );
        try
        {
            backend.ensureCollections(List.of(compatibleDescriptor(), incompatible));
            throw new AssertionError("expected schema conflict");
        }
        catch (Backend.ConflictException error)
        {
            require(
                error.kind() == Backend.ConflictKind.SCHEMA_CONFLICT, "expected schema conflict"
            );
        }

        backend.ensureCollection(alternateCompatibleDescriptor());
    }

    private static void extractsIndexKeysDeterministically() throws Backend.BackendException
    {
        var index = new Backend.IndexDescriptor(
            "by_status", List.of("status", "priority", "archived", "missing", "cleared"), false
        );
        var document = Json.object(
            Map.of(
                "status", Json.string("Open"),
                "priority", Json.integer(3L),
                "archived", Json.bool(false),
                "cleared", Json.nullValue()
            )
        );

        var key = InMemoryTransaction.extractIndexKey(document, index);
        require(
            key.equals(List.of("string:Open", "int:3", "bool:false", "missing:", "null:")),
            "expected typed index key encoding"
        );

        try
        {
            InMemoryTransaction.extractIndexKey(
                Json.object(Map.of("status", Json.array(List.of(Json.string("Open"))))), index
            );
            throw new AssertionError("expected scalar index field error");
        }
        catch (Backend.BackendException expected)
        {
            require(
                expected.getMessage().contains("scalar JSON values"),
                "expected scalar value diagnostic"
            );
        }
    }

    private static Backend.CollectionDescriptor baseDescriptor()
    {
        return new Backend.CollectionDescriptor(
            "orders",
            List.of(
                new Backend.FieldDescriptor("tenant_id", Backend.FieldType.STRING, "string", true),
                new Backend.FieldDescriptor("status", Backend.FieldType.STRING, "string", false)
            ),
            List.of("tenant_id"),
            List.of(new Backend.IndexDescriptor("by_status", List.of("status"), false)), 1L
        );
    }

    private static Backend.CollectionDescriptor compatibleDescriptor()
    {
        var fields = new ArrayList<>(baseDescriptor().fields());
        fields.add(
            new Backend.FieldDescriptor("description", Backend.FieldType.STRING, "string", false)
        );
        return descriptor(fields, baseDescriptor().keyFields(), baseDescriptor().indexes(), 2L);
    }

    private static Backend.CollectionDescriptor alternateCompatibleDescriptor()
    {
        var fields = new ArrayList<>(baseDescriptor().fields());
        fields.add(new Backend.FieldDescriptor("notes", Backend.FieldType.STRING, "string", false));
        return descriptor(fields, baseDescriptor().keyFields(), baseDescriptor().indexes(), 2L);
    }

    private static Backend.CollectionDescriptor descriptor(
        List<Backend.FieldDescriptor> fields,
        List<String> keyFields,
        List<Backend.IndexDescriptor> indexes,
        long schemaVersion
    )
    {
        return new Backend.CollectionDescriptor(
            "orders", fields, keyFields, indexes, schemaVersion
        );
    }

    private static void require(
        boolean condition,
        String message
    )
    {
        if (!condition)
        {
            throw new AssertionError(message);
        }
    }
}
