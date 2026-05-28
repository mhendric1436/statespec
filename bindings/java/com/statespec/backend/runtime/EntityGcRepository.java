package com.statespec.backend.runtime;

import com.statespec.backend.Backend;
import java.util.List;

public interface EntityGcRepository
{
    record EligibilityRequest(
        EntityGcTypes.Descriptor descriptor,
        String now,
        int limit
    )
    {
    }

    record FinalizeRequest(
        EntityGcTypes.Descriptor descriptor,
        String key,
        String now
    )
    {
    }

    List<Backend.VersionedRecord> listGcEligibleTx(
        Backend.Transaction tx,
        EligibilityRequest request
    ) throws Backend.BackendException;

    void finalizeGcTx(
        Backend.Transaction tx,
        FinalizeRequest request
    ) throws Backend.BackendException;
}
