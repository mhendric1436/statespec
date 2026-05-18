package com.statespec.backend;

import com.statespec.backend.Backend.BackendException;
import com.statespec.backend.Backend.Transaction;
import com.statespec.backend.Backend.VersionedRecord;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public interface ExternalSystem
{
    record MetadataKeyValue(
        String field,
        Json value
    )
    {
    }

    record MetadataLookup(
        String externalSystem,
        String metadataEntity,
        Optional<String> tenantField,
        String profileField,
        List<String> keyFields,
        List<MetadataKeyValue> keyValues,
        List<String> requiredFields
    )
    {
        public boolean keyComplete()
        {
            return missingMetadataKeyFields(this).isEmpty();
        }
    }

    record MetadataResolution(
        VersionedRecord record,
        List<String> missingRequiredFields
    )
    {
        public boolean complete()
        {
            return missingRequiredFields.isEmpty();
        }
    }

    static List<String> missingRequiredMetadataFields(
        Json document,
        List<String> requiredFields
    )
    {
        List<String> missing = new ArrayList<>();
        for (String field : requiredFields)
        {
            if (document.find(field).isEmpty())
            {
                missing.add(field);
            }
        }
        return missing;
    }

    static List<String> missingMetadataKeyFields(MetadataLookup lookup)
    {
        List<String> missing = new ArrayList<>();
        for (String keyField : lookup.keyFields())
        {
            boolean found = false;
            for (MetadataKeyValue keyValue : lookup.keyValues())
            {
                if (keyValue.field().equals(keyField))
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                missing.add(keyField);
            }
        }
        return missing;
    }

    Optional<MetadataResolution> resolveMetadata(
        Backend backend,
        MetadataLookup lookup
    ) throws BackendException;

    Optional<MetadataResolution> resolveMetadataTx(
        Transaction tx,
        MetadataLookup lookup
    ) throws BackendException;
}
