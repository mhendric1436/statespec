package com.statespec.generated;

import com.statespec.backend.memory.InMemoryBackend;
import com.statespec.backend.runtime.LeaseStore;
import com.statespec.backend.runtime.QueueStore;
import com.statespec.backend.runtime.WorkflowStore;
import com.statespec.generated.runtime.RuntimeRegistration;

public final class RegistrationRestartFixture
{
    private RegistrationRestartFixture() {}

    public static void main(String[] args) throws Exception
    {
        var backend = new InMemoryBackend();

        RuntimeRegistration.bootstrapRuntimeCatalog(
            backend, new QueueStore(), new LeaseStore(), new WorkflowStore()
        );
        RuntimeRegistration.bootstrapRuntimeCatalog(
            backend, new QueueStore(), new LeaseStore(), new WorkflowStore()
        );
    }
}
