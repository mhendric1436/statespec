use std::thread;
use std::time::Duration;

use statespec_generated::api_handler_registry::DefaultApiHandler;
use statespec_generated::api_process::{ApiProcess, ApiProcessConfig};
use statespec_generated::api_transport::LocalBlockingApiTransport;
use statespec_generated::memory_backend::InMemoryBackend;

#[test]
fn generated_api_process_constructs_multiple_servers_and_stops() {
    let backend = InMemoryBackend::new();
    let handler = DefaultApiHandler {
        backend: backend.clone(),
    };
    let transport = LocalBlockingApiTransport::new();
    let config = ApiProcessConfig::all_servers();
    assert_eq!(config.server_names.len(), 2);

    let process = std::sync::Arc::new(
        ApiProcess::new(config, backend, handler, transport).expect("process construction failed"),
    );
    assert_eq!(process.applications.len(), 2);

    let runner = {
        let process = process.clone();
        thread::spawn(move || process.run())
    };
    process.request_stop();

    for _ in 0..20 {
        if runner.is_finished() {
            break;
        }
        thread::sleep(Duration::from_millis(100));
    }
    assert!(runner.is_finished(), "API process did not stop");
    runner
        .join()
        .expect("API process runner panicked")
        .expect("API process did not stop cleanly");
}
