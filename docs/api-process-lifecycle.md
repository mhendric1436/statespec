# API Process Lifecycle

Generated API applications use the same lifecycle contract across C++, Go, Java, and
Rust. The contract makes `ApiProcess` responsible for process threading, bootstrap,
shutdown signaling, and completion reporting. Generated `main` functions should not
create ad hoc threads around `run()`; they should compose the generated process, start
it, install stop handling, and join it.

## Ownership

Generated code owns:

- API process configuration
- backend, handler, API server, and transport composition
- bootstrap of generated runtime catalogs
- optional entity GC background worker lifecycle when API GC is enabled
- the background thread, goroutine, or task that runs the API process
- stop signaling and join semantics
- deterministic lifecycle errors such as double start

User-owned runtime code owns:

- concrete network transport selection
- HTTP/RPC framework integration
- framework request parsing and response serialization
- authentication, authorization, tenancy policy, and middleware
- durable backend adapter selection
- concrete API handler implementations

The generated local/no-op transport exists to validate startup and shutdown shape. It
must not be treated as an opinionated HTTP backend.

## Required API Shape

Each language binding should expose the same lifecycle concepts with idiomatic names:

| Concept | C++ | Go | Java | Rust |
|---|---|---|---|---|
| Start background execution | `start()` | `Start(ctx)` | `start()` | `start()` |
| Signal shutdown | `request_stop()` | `RequestStop()` | `requestStop()` | `request_stop()` |
| Wait for completion | `join()` | `Join()` | `join()` | `join()` |
| Internal blocking run loop | `run()` | `run(ctx)` or `Run(ctx)` | `run()` | `run()` |

The blocking `run()` method may remain available for implementation reuse, but it is not
the primary generated application lifecycle entrypoint. Generated `main` should use:

1. construct backend, handler, transport, config, and `ApiProcess`
2. call `start()`
3. install signal or shutdown handling that calls `request_stop()`
4. call `join()`
5. return the reported status or error

## Required Semantics

`start()` starts the API process on an owned background thread, goroutine, or task. It
must bootstrap the generated application before entering the transport run loop, or
otherwise fail before reporting the process as started.

## Startup And Bootstrap

Generated `main` functions do not call `bootstrap()` directly. They construct the
backend, handler, local transport, config, and `ApiProcess`, then call `start()` and
`join()`. Bootstrap runs inside the generated process execution path when
`bootstrap_on_start` / `BootstrapOnStart` is enabled, which is the default generated
configuration.

If entity GC artifacts are generated and API GC is enabled in process config, `start()`
also starts the registered GC workers. `request_stop()` stops both the transport and GC
workers, and `join()` waits for both to finish. In mixed API + Worker deployments, the
runtime should disable API GC when the Worker tier is selected as the GC host.

The cross-language startup flow is:

| Language | Startup path |
|---|---|
| C++ | `main()` -> `process.start()` -> owned `std::thread` -> private `run()` -> `bootstrap()` -> `transport.run()` |
| Go | `main()` -> `process.Start(ctx)` -> owned goroutine -> `Run(ctx)` -> `Bootstrap(ctx)` -> `Transport.Run(ctx)` |
| Java | `ApiMain.run()` -> `process.start()` -> owned `Thread` -> `run()` -> `bootstrap()` -> `transport.run()` |
| Rust | `main::run()` -> `Arc<ApiProcess>::start()` -> owned `std::thread` -> `run()` -> `bootstrap()` -> `transport.run()` |

`bootstrap()` registers generated runtime catalogs through each generated API
application before the transport begins blocking or serving work. It is not business
initialization and it is not concrete HTTP/RPC startup. In generated local runs it
registers the spec-driven runtime definitions needed by the generated app, then the
local/no-op transport blocks until a stop request is received.

The generated bootstrap methods are idempotent. Calling `bootstrap()` manually before
`start()` is allowed for advanced composition, but normal generated entrypoints should
rely on the default `bootstrap_on_start` behavior. If a user/runtime disables
`bootstrap_on_start`, that runtime becomes responsible for calling `bootstrap()` before
starting a real transport.

`request_stop()` is idempotent and harmless before `start()`. It records a stop request
and asks the configured transport to unblock if the transport has started.

`join()` waits for the owned background execution to finish and returns the final
status/error. Calling `join()` before `start()` must behave consistently across
languages: it should return a clear not-started error rather than silently running work
on the caller thread.

Starting the same `ApiProcess` twice must be rejected deterministically. The second
start attempt should return or throw a lifecycle error without launching another worker.

The generated local transport must start successfully and block until shutdown is
requested. It should unblock when `request_stop()` is called so `join()` can complete.
The generated transport interface must expose the stop method directly:

- C++: `request_stop()`
- Go: `RequestStop()`
- Java: `requestStop()`
- Rust: `request_stop()`

`ApiProcess.request_stop()` must delegate to that transport stop method. Generated
process shutdown must not rely on thread interruption, cancellation, or process exit as
the primary way to unblock the transport.

## Implementation Notes

Lifecycle state should be protected by the language's normal synchronization primitive:

- C++: `std::mutex`, `std::thread`, and stored status
- Go: `sync.Mutex`, `context.Context`, cancellation, and a completion channel
- Java: synchronized state or atomics plus a `Thread`
- Rust: `Arc`, `Mutex`, and `std::thread::JoinHandle`

Generated process destructors/finalizers should not be the only shutdown mechanism.
Callers should explicitly request stop and join during normal shutdown.
