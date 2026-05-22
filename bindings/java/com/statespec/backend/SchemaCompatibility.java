package com.statespec.backend;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public final class SchemaCompatibility
{
    private SchemaCompatibility() {}

    public enum Status
    {
        IDENTICAL,
        COMPATIBLE,
        INCOMPATIBLE
    }

    public enum Reason
    {
        SCHEMA_VERSION_DECREASED,
        SCHEMA_VERSION_NOT_INCREMENTED,
        COLLECTION_NAME_CHANGED,
        KEY_FIELDS_CHANGED,
        FIELD_REMOVED,
        FIELD_RENAMED,
        FIELD_TYPE_CHANGED,
        FIELD_TYPE_NAME_CHANGED,
        REQUIRED_FIELD_ADDED,
        FIELD_BECAME_REQUIRED,
        INDEX_REMOVED,
        INDEX_RENAMED,
        INDEX_FIELDS_CHANGED,
        INDEX_UNIQUENESS_CHANGED,
        UNIQUE_INDEX_ADDED
    }

    public record Difference(
        Reason reason,
        String path,
        String message
    )
    {
    }

    public record Result(
        Status status,
        List<Difference> differences
    )
    {
        public boolean compatible()
        {
            return status != Status.INCOMPATIBLE;
        }
    }

    public static Result compareCollectionDescriptors(
        Backend.CollectionDescriptor existing,
        Backend.CollectionDescriptor requested
    )
    {
        if (existing.equals(requested))
        {
            return new Result(Status.IDENTICAL, List.of());
        }

        var differences = new ArrayList<Difference>();
        if (!existing.name().equals(requested.name()))
        {
            differences.add(new Difference(
                Reason.COLLECTION_NAME_CHANGED, "name",
                "collection name changed from '" + existing.name() + "' to '" + requested.name() +
                    "'"
            ));
        }
        if (requested.schemaVersion() < existing.schemaVersion())
        {
            differences.add(new Difference(
                Reason.SCHEMA_VERSION_DECREASED, "schema_version", "schema_version decreased"
            ));
        }
        else if (requested.schemaVersion() == existing.schemaVersion())
        {
            differences.add(new Difference(
                Reason.SCHEMA_VERSION_NOT_INCREMENTED, "schema_version",
                "schema_version must increase when descriptor shape changes"
            ));
        }
        if (!existing.keyFields().equals(requested.keyFields()))
        {
            differences.add(
                new Difference(Reason.KEY_FIELDS_CHANGED, "key_fields", "key_fields changed")
            );
        }

        for (var existingField : existing.fields())
        {
            Optional<Backend.FieldDescriptor> requestedField =
                findFieldDescriptor(requested.fields(), existingField.name());
            if (requestedField.isEmpty())
            {
                differences.add(new Difference(
                    Reason.FIELD_REMOVED, "fields." + existingField.name(),
                    "field '" + existingField.name() + "' was removed"
                ));
                continue;
            }
            var field = requestedField.orElseThrow();
            if (existingField.type() != field.type())
            {
                differences.add(new Difference(
                    Reason.FIELD_TYPE_CHANGED, "fields." + existingField.name(),
                    "field '" + existingField.name() + "' type changed"
                ));
            }
            if (!existingField.typeName().equals(field.typeName()))
            {
                differences.add(new Difference(
                    Reason.FIELD_TYPE_NAME_CHANGED, "fields." + existingField.name(),
                    "field '" + existingField.name() + "' type_name changed"
                ));
            }
            if (!existingField.required() && field.required())
            {
                differences.add(new Difference(
                    Reason.FIELD_BECAME_REQUIRED, "fields." + existingField.name(),
                    "field '" + existingField.name() + "' became required"
                ));
            }
        }

        for (var requestedField : requested.fields())
        {
            if (findFieldDescriptor(existing.fields(), requestedField.name()).isEmpty() &&
                requestedField.required())
            {
                differences.add(new Difference(
                    Reason.REQUIRED_FIELD_ADDED, "fields." + requestedField.name(),
                    "required field '" + requestedField.name() + "' was added"
                ));
            }
        }

        for (var existingIndex : existing.indexes())
        {
            Optional<Backend.IndexDescriptor> requestedIndex =
                findIndexDescriptor(requested.indexes(), existingIndex.name());
            if (requestedIndex.isEmpty())
            {
                differences.add(new Difference(
                    Reason.INDEX_REMOVED, "indexes." + existingIndex.name(),
                    "index '" + existingIndex.name() + "' was removed"
                ));
                continue;
            }
            var index = requestedIndex.orElseThrow();
            if (!existingIndex.fields().equals(index.fields()))
            {
                differences.add(new Difference(
                    Reason.INDEX_FIELDS_CHANGED, "indexes." + existingIndex.name(),
                    "index '" + existingIndex.name() + "' fields changed"
                ));
            }
            if (existingIndex.unique() != index.unique())
            {
                differences.add(new Difference(
                    Reason.INDEX_UNIQUENESS_CHANGED, "indexes." + existingIndex.name(),
                    "index '" + existingIndex.name() + "' uniqueness changed"
                ));
            }
        }

        for (var requestedIndex : requested.indexes())
        {
            if (findIndexDescriptor(existing.indexes(), requestedIndex.name()).isEmpty() &&
                requestedIndex.unique())
            {
                differences.add(new Difference(
                    Reason.UNIQUE_INDEX_ADDED, "indexes." + requestedIndex.name(),
                    "unique index '" + requestedIndex.name() + "' was added"
                ));
            }
        }

        if (!differences.isEmpty())
        {
            return new Result(Status.INCOMPATIBLE, List.copyOf(differences));
        }
        return new Result(Status.COMPATIBLE, List.of());
    }

    public static void validateCollectionDescriptorUpgrade(
        Backend.CollectionDescriptor existing,
        Backend.CollectionDescriptor requested
    ) throws Backend.ConflictException
    {
        var result = compareCollectionDescriptors(existing, requested);
        if (result.compatible())
        {
            return;
        }

        var message = "collection schema upgrade is incompatible";
        if (!result.differences().isEmpty())
        {
            message = message + ": " + result.differences().get(0).message();
        }
        throw new Backend.ConflictException(Backend.ConflictKind.SCHEMA_CONFLICT, message);
    }

    private static Optional<Backend.FieldDescriptor> findFieldDescriptor(
        List<Backend.FieldDescriptor> fields,
        String name
    )
    {
        return fields.stream().filter(field -> field.name().equals(name)).findFirst();
    }

    private static Optional<Backend.IndexDescriptor> findIndexDescriptor(
        List<Backend.IndexDescriptor> indexes,
        String name
    )
    {
        return indexes.stream().filter(index -> index.name().equals(name)).findFirst();
    }
}
