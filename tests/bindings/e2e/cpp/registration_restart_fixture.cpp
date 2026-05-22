#include "common/descriptors.hpp"
#include "common/memory/backend.hpp"
#include "common/runtime/lease_store.hpp"
#include "common/runtime/queue_store.hpp"
#include "common/runtime/workflow_store.hpp"

int main()
{
    statespec::backend::memory::InMemoryBackend backend;

    {
        statespec::backend::runtime::RuntimeQueueStore queues;
        statespec::backend::runtime::RuntimeLeaseStore leases;
        statespec::backend::runtime::RuntimeWorkflowStore workflows;
        statespec_generated::bootstrap_runtime_catalog(backend, queues, leases, workflows);
    }

    {
        statespec::backend::runtime::RuntimeQueueStore queues;
        statespec::backend::runtime::RuntimeLeaseStore leases;
        statespec::backend::runtime::RuntimeWorkflowStore workflows;
        statespec_generated::bootstrap_runtime_catalog(backend, queues, leases, workflows);
    }
}
