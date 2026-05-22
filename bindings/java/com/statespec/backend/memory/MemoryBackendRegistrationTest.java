package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import java.util.ArrayList;
import java.util.List;

public final class MemoryBackendRegistrationTest
{
    public static void main(String[] args) throws Backend.BackendException
    {
        enforcesCompatibleCollectionRegistration();
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
