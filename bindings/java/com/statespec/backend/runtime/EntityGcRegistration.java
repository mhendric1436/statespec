package com.statespec.backend.runtime;

import com.statespec.backend.Backend;
import com.statespec.backend.Json;
import java.util.ArrayList;
import java.util.List;

public final class EntityGcRegistration
{
    private EntityGcRegistration() {}

    @FunctionalInterface
    public interface EntityGcTask {
        void run(String now) throws Backend.BackendException;
    }

    @FunctionalInterface
    public interface Registrar {
        void add(EntityGcTask task) throws Backend.BackendException;
    }

    public static final class DefaultEntityGcRepository implements EntityGcRepository
    {
        @Override
        public List<Backend.VersionedRecord> listGcEligibleTx(
            Backend.Transaction tx,
            EligibilityRequest request
        ) throws Backend.BackendException
        {
            int limit = request.limit() > 0 ? request.limit() : 100;
            List<Backend.VersionedRecord> eligible = new ArrayList<>();
            for (var record : tx.query(request.descriptor().collection(), new Backend.Query.All()))
            {
                if (eligible.size() >= limit)
                {
                    break;
                }
                var status = record.document().find(request.descriptor().statusField());
                if (status.isEmpty() || !(status.get() instanceof Json.StringValue value))
                {
                    continue;
                }
                if (isTerminalGcState(request.descriptor(), value.value()))
                {
                    eligible.add(record);
                }
            }
            return eligible;
        }

        @Override
        public void finalizeGcTx(
            Backend.Transaction tx,
            FinalizeRequest request
        ) throws Backend.BackendException
        {
            tx.erase(request.descriptor().collection(), request.key());
        }
    }

    public static void registerEntityGcWorkers(
        Registrar registrar,
        Backend backend
    ) throws Backend.BackendException
    {
        registerEntityGcWorkers(registrar, backend, List.of());
    }

    public static void registerEntityGcWorkers(
        Registrar registrar,
        Backend backend,
        List<EntityGcTypes.Descriptor> descriptors
    ) throws Backend.BackendException
    {
        for (var descriptor : descriptors)
        {
            var worker = new EntityGcWorkers.Worker(
                descriptor, new DefaultEntityGcRepository(), new EntityGcWorkers.WorkerConfig(0)
            );
            registrar.add(now -> worker.runOnce(backend, now));
        }
    }

    private static boolean isTerminalGcState(
        EntityGcTypes.Descriptor descriptor,
        String status
    )
    {
        for (var terminal : descriptor.terminalStates())
        {
            if (terminal.state().equals(status))
            {
                return true;
            }
        }
        return false;
    }
}
