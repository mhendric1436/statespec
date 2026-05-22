package com.statespec.backend;

import java.util.ArrayList;
import java.util.List;

public final class SchemaCompatibilityTest
{
    public static void main(String[] args)
    {
        allowsIdenticalAndCompatibleChanges();
        rejectsIncompatibleChanges();
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

    private static void allowsIdenticalAndCompatibleChanges()
    {
        var base = baseDescriptor();
        var identical = SchemaCompatibility.compareCollectionDescriptors(base, base);
        require(
            identical.status() == SchemaCompatibility.Status.IDENTICAL && identical.compatible(),
            "identical descriptor should be compatible"
        );

        var optionalFields = new ArrayList<>(base.fields());
        optionalFields.add(
            new Backend.FieldDescriptor("description", Backend.FieldType.STRING, "string", false)
        );
        var optionalField = descriptor(optionalFields, base.keyFields(), base.indexes(), 2L);
        var optionalResult = SchemaCompatibility.compareCollectionDescriptors(base, optionalField);
        require(
            optionalResult.status() == SchemaCompatibility.Status.COMPATIBLE,
            "optional field should be compatible"
        );

        var nonUniqueIndexes = new ArrayList<>(base.indexes());
        nonUniqueIndexes.add(
            new Backend.IndexDescriptor("by_tenant_status", List.of("tenant_id", "status"), false)
        );
        var nonUniqueIndex = descriptor(base.fields(), base.keyFields(), nonUniqueIndexes, 2L);
        require(
            SchemaCompatibility.compareCollectionDescriptors(base, nonUniqueIndex).compatible(),
            "non-unique index should be compatible"
        );
    }

    private static void rejectsIncompatibleChanges()
    {
        var base = baseDescriptor();

        var sameVersionFields = new ArrayList<>(base.fields());
        sameVersionFields.add(
            new Backend.FieldDescriptor("description", Backend.FieldType.STRING, "string", false)
        );
        requireReason(
            descriptor(sameVersionFields, base.keyFields(), base.indexes(), 1L),
            SchemaCompatibility.Reason.SCHEMA_VERSION_NOT_INCREMENTED
        );
        requireReason(
            descriptor(sameVersionFields, base.keyFields(), base.indexes(), 0L),
            SchemaCompatibility.Reason.SCHEMA_VERSION_DECREASED
        );
        requireReason(
            descriptor(base.fields(), List.of("tenant_id", "order_id"), base.indexes(), 2L),
            SchemaCompatibility.Reason.KEY_FIELDS_CHANGED
        );
        requireReason(
            descriptor(base.fields().subList(0, 1), base.keyFields(), base.indexes(), 2L),
            SchemaCompatibility.Reason.FIELD_REMOVED
        );

        requireReason(
            descriptor(
                List.of(
                    base.fields().get(0),
                    new Backend.FieldDescriptor("status", Backend.FieldType.INT, "string", false)
                ),
                base.keyFields(), base.indexes(), 2L
            ),
            SchemaCompatibility.Reason.FIELD_TYPE_CHANGED
        );
        requireReason(
            descriptor(
                List.of(
                    base.fields().get(0),
                    new Backend.FieldDescriptor("status", Backend.FieldType.STRING, "Status", false)
                ),
                base.keyFields(), base.indexes(), 2L
            ),
            SchemaCompatibility.Reason.FIELD_TYPE_NAME_CHANGED
        );
        requireReason(
            descriptor(
                List.of(
                    base.fields().get(0),
                    new Backend.FieldDescriptor("status", Backend.FieldType.STRING, "string", true)
                ),
                base.keyFields(), base.indexes(), 2L
            ),
            SchemaCompatibility.Reason.FIELD_BECAME_REQUIRED
        );

        var requiredFieldAdded = new ArrayList<>(base.fields());
        requiredFieldAdded.add(
            new Backend.FieldDescriptor("priority", Backend.FieldType.INT, "int", true)
        );
        requireReason(
            descriptor(requiredFieldAdded, base.keyFields(), base.indexes(), 2L),
            SchemaCompatibility.Reason.REQUIRED_FIELD_ADDED
        );
        requireReason(
            descriptor(base.fields(), base.keyFields(), List.of(), 2L),
            SchemaCompatibility.Reason.INDEX_REMOVED
        );
        requireReason(
            descriptor(
                base.fields(), base.keyFields(),
                List.of(
                    new Backend.IndexDescriptor("by_status", List.of("tenant_id", "status"), false)
                ),
                2L
            ),
            SchemaCompatibility.Reason.INDEX_FIELDS_CHANGED
        );
        requireReason(
            descriptor(
                base.fields(), base.keyFields(),
                List.of(new Backend.IndexDescriptor("by_status", List.of("status"), true)), 2L
            ),
            SchemaCompatibility.Reason.INDEX_UNIQUENESS_CHANGED
        );

        var uniqueIndexAdded = new ArrayList<>(base.indexes());
        uniqueIndexAdded.add(new Backend.IndexDescriptor("unique_status", List.of("status"), true));
        requireReason(
            descriptor(base.fields(), base.keyFields(), uniqueIndexAdded, 2L),
            SchemaCompatibility.Reason.UNIQUE_INDEX_ADDED
        );
    }

    private static void requireReason(
        Backend.CollectionDescriptor requested,
        SchemaCompatibility.Reason reason
    )
    {
        var result = SchemaCompatibility.compareCollectionDescriptors(baseDescriptor(), requested);
        require(
            result.status() == SchemaCompatibility.Status.INCOMPATIBLE,
            "descriptor should be incompatible"
        );
        require(
            result.differences().stream().anyMatch(difference -> difference.reason() == reason),
            "expected incompatibility reason " + reason
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
