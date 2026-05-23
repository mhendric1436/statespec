package com.statespec.backend.runtime;

import com.statespec.backend.Backend;

public final class EntityGcWorkers
{
    private EntityGcWorkers() {}

    public record WorkerConfig(int batchLimit)
    {
        public int effectiveBatchLimit()
        {
            return batchLimit > 0 ? batchLimit : 100;
        }
    }

    public record RunResult(
        int scanned,
        int finalized
    )
    {
    }

    public static final class Worker
    {
        private final EntityGcDescriptors.Descriptor descriptor;
        private final EntityGcRepository repository;
        private final WorkerConfig config;

        public Worker(
            EntityGcDescriptors.Descriptor descriptor,
            EntityGcRepository repository,
            WorkerConfig config
        )
        {
            this.descriptor = descriptor;
            this.repository = repository;
            this.config = config;
        }

        public RunResult runOnce(
            Backend backend,
            String now
        ) throws Backend.BackendException
        {
            var tx = backend.begin();
            try
            {
                var result = runOnceTx(tx, now);
                backend.commit(tx);
                return result;
            }
            catch (Backend.BackendException error)
            {
                tx.abort();
                throw error;
            }
        }

        public RunResult runOnceTx(
            Backend.Transaction tx,
            String now
        ) throws Backend.BackendException
        {
            var records = repository.listGcEligibleTx(
                tx, new EntityGcRepository.EligibilityRequest(
                        descriptor, now, config.effectiveBatchLimit()
                    )
            );
            int finalized = 0;
            for (var record : records)
            {
                repository.finalizeGcTx(
                    tx, new EntityGcRepository.FinalizeRequest(descriptor, record.key(), now)
                );
                finalized++;
            }
            return new RunResult(records.size(), finalized);
        }

        public EntityGcDescriptors.Descriptor descriptor()
        {
            return descriptor;
        }
    }
}
