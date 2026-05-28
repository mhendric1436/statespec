use std::time::Duration;

use statespec_generated::api_handler_registry::DefaultApiHandler;
use statespec_generated::api_process::{ApiProcess, ApiProcessConfig};
use statespec_generated::api_transport::LocalBlockingApiTransport;
use statespec_generated::memory_backend::InMemoryBackend;

#[test]
fn generated_api_process_constructs_multiple_servers_and_stops() {
    let backend = InMemoryBackend::new();
    let handler = DefaultApiHandler::new(backend.clone());
    let transport = LocalBlockingApiTransport::new();
    let config = ApiProcessConfig::all_servers();
    assert_eq!(config.server_names.len(), 2);

    let process = std::sync::Arc::new(
        ApiProcess::new(config.clone(), backend.clone(), handler.clone(), transport)
            .expect("process construction failed"),
    );
    assert_eq!(process.applications.len(), 2);

    let not_started = std::sync::Arc::new(
        ApiProcess::new(
            config.clone(),
            backend.clone(),
            handler.clone(),
            LocalBlockingApiTransport::new(),
        )
        .expect("not-started process construction failed"),
    );
    not_started.request_stop();
    assert!(not_started.join().is_err(), "join before start should fail");

    process.start().expect("API process did not start cleanly");
    assert!(process.is_running(), "API process did not report running");
    assert!(process.start().is_err(), "API process allowed double start");
    process.request_stop();

    for _ in 0..20 {
        if !process.is_running() {
            break;
        }
        std::thread::sleep(Duration::from_millis(100));
    }
    process.join().expect("API process did not stop cleanly");
    assert!(
        !process.is_running(),
        "API process still reports running after join"
    );
}
