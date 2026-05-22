package com.statespec.backend;

import java.util.List;

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
}
