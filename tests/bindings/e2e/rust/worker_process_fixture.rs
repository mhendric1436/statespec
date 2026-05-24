use std::sync::Arc;
use std::thread;
use std::time::Duration;

use statespec_generated::memory_backend::InMemoryBackend;
use statespec_generated::worker_process::WorkerProcess;
use statespec_generated::worker_runtime::WorkerRuntime;

#[test]
fn generated_worker_process_lifecycle() {
    let backend = InMemoryBackend::new();
    let runtime = WorkerRuntime::new(backend);
    let process = Arc::new(WorkerProcess::new(runtime));

    assert!(
        process.join().is_err(),
        "joining a WorkerProcess before start should fail"
    );
    process.request_stop();

    process.start().expect("starting WorkerProcess failed");
    assert!(
        process.start().is_err(),
        "starting WorkerProcess twice should fail"
    );
    thread::sleep(Duration::from_millis(10));
    assert!(
        process.is_running(),
        "started WorkerProcess should report running"
    );

    process.request_stop();
    process
        .join()
        .expect("stopped WorkerProcess should join cleanly");
}
