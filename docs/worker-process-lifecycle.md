# Generated Worker Process Lifecycle

Generated Worker apps have a production-shaped process lifecycle without choosing a
queue framework, scheduler framework, or service supervisor. The generated
`WorkerProcess` owns threading, startup, stop, and join semantics. User code supplies
workflow step handlers and the concrete backend adapter.

## Contract

Across C++, Go, Java, and Rust, the generated Worker process follows this lifecycle:

1. Construct the backend, `WorkerRuntime`, workflow step handler, and `WorkerProcess`.
2. Call `start()` / `Start(...)` to launch the Worker process on an owned background
   thread or goroutine.
3. Bootstrap runtime definitions when configured to do so.
4. Start generated entity GC workers when Worker GC is enabled.
5. Start generated worker polling loops for each worker context with an `executes`
   workflow binding.
6. Stop workflow loops and GC workers on `request_stop()` / `RequestStop()`.
7. Wait for all generated background work to finish in `join()` / `Join()`.

Starting twice is rejected deterministically. Requesting stop before start is harmless.
Joining before start returns a clear error or non-zero status, depending on the target
language convention.

## Worker Loops

Generated worker loops are derived from `worker` declarations and `WorkerContext`
records. For every context with `executes`, the process starts `concurrency` polling
loops. Each loop asks `WorkerRuntime.run_once` to claim the next runnable workflow
execution for the declared workflow. The runtime delegates to `WorkflowRunner`, which
owns claim, keep alive, typed handler dispatch, and applying the handler-returned
complete/fail/cancel result.

The empty workflow execution id used by the polling loop means "claim the next runnable
execution for this workflow name and version." Direct tests and specialized adapters may
still pass a concrete workflow execution id to run a known execution.

## Claim Tokens And Keep-Alive

The generated C++, Go, Java, and Rust `WorkflowRunner.run_once` implementations start a periodic
keep-alive controller after a workflow step is claimed and before the typed step handler
is invoked. The controller refreshes the claim heartbeat on a background thread or
goroutine while the handler transaction is open, then stops before the runner applies
the handler result with `complete`, `fail`, or `cancel`.

Claiming a workflow step assigns a claim token to the workflow execution and writes a
matching workflow heartbeat record. Keep-alive requests must include that token. The
workflow store validates the token and refreshes the heartbeat record instead of
updating the workflow execution document. Completing, failing, or canceling the step
validates the same token and clears the heartbeat. This keeps background heartbeats from
racing the open handler transaction on the workflow execution document version.

Periodic keep-alive implementations must reuse this heartbeat path. A background
keep-alive must not update the same workflow execution document version that the open
handler transaction reads and later commits.

## Transaction Boundaries

`WorkflowRunner.run_once` uses claim ownership and step finalization as separate OCC
boundaries:

1. Claim the step with a backend-managed workflow-store call and commit that claim.
2. Keep the claim alive with backend-managed heartbeat updates. C++, Go, Java, and Rust
   do this periodically while the handler runs.
3. Begin a second transaction, re-read and revalidate the claimed workflow execution,
   invoke the typed step handler with that transaction, apply the handler result with
   `complete`, `fail`, or `cancel` through the workflow store `Tx` API, and commit.

The second transaction is intentionally allowed to stay open while the handler runs.
That keeps handler-owned persisted state changes and workflow advancement in one OCC
commit. The required tradeoff is idempotency: remote calls made from a handler must be
safe to repeat if the process crashes, the lease expires, or the final transaction
conflicts. The generated claim token is a runtime ownership token, not a persisted
business idempotency key. Prefer external idempotency keys derived from:

```text
workflow_execution_id + ":" + current_step + ":" + attempt
```

The OCC transaction covers StateSpec backend state only. External systems still require
their own idempotency, retry, timeout, and reconciliation behavior.

## User-Owned Code

Generated code provides default workflow-specific handler implementations so generated
apps link and exercise lifecycle tests. Production applications should register concrete
workflow-specific handlers in the generated workflow step invoker map. Each handler
method returns a generated workflow step result: complete with an optional next step,
fail with a reason, or cancel with a reason.

User-owned workflow step handlers should be idempotent. Any persisted state read that
affects a worker decision must use the generated backend transaction model and be
coordinated with the write, queue, workflow, log, or metric operation that consumes the
read. In generated workflow runners, that means using the transaction passed to the step
handler and letting the runner commit that transaction with the workflow step result.

## Garbage Collection

Entity GC runtime artifacts live in the common tier. The Worker process starts and joins
Worker-hosted GC workers through the same lifecycle as workflow polling loops. Worker
GC is enabled by default for standalone Worker apps. In mixed API + Worker deployments,
enable GC on only one tier to avoid duplicate background scans.

Generated Worker startup registers one GC task per descriptor from the Worker-owned GC
catalog before `WorkerProcess.start()`. The Worker catalog is a deployment-tier binding
artifact, not a common global descriptor list. GC remains limited to generic OCC backend
and transaction primitives. It must not depend on workflows, queues, leases, feature
flags, logs, metrics, or a separate scheduler abstraction.

## Runtime Ownership

`WorkerProcess` owns process lifecycle. `WorkerRuntime` owns runtime store composition,
definition registration, GC worker registration, and one-step workflow execution. The
generic backend and transaction interfaces remain the only persistence boundary.
