use statespec_generated::descriptors::bootstrap_runtime_catalog;
use statespec_generated::memory_backend::InMemoryBackend;
use statespec_generated::runtime_leases::RuntimeLeaseStore;
use statespec_generated::runtime_queues::RuntimeQueueStore;
use statespec_generated::runtime_workflows::RuntimeWorkflowStore;

#[test]
fn generated_runtime_catalog_bootstrap_is_restart_safe() {
    let backend = InMemoryBackend::new();

    bootstrap_runtime_catalog(
        &backend,
        &RuntimeQueueStore::new(),
        &RuntimeLeaseStore::new(),
        &RuntimeWorkflowStore::new(),
    )
    .unwrap();
    bootstrap_runtime_catalog(
        &backend,
        &RuntimeQueueStore::new(),
        &RuntimeLeaseStore::new(),
        &RuntimeWorkflowStore::new(),
    )
    .unwrap();
}
