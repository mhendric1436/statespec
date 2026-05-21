package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.FeatureFlag;
import com.statespec.backend.Json;
import com.statespec.backend.Lease;
import com.statespec.backend.Log;
import com.statespec.backend.Metric;
import com.statespec.backend.Queue;
import com.statespec.backend.Workflow;
import com.statespec.backend.runtime.RuntimeFeatureFlagStore;
import com.statespec.backend.runtime.RuntimeLeaseStore;
import com.statespec.backend.runtime.RuntimeLogSink;
import com.statespec.backend.runtime.RuntimeMetricSink;
import com.statespec.backend.runtime.RuntimeQueueStore;
import com.statespec.backend.runtime.RuntimeWorkflowStore;
import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public final class MemoryBackendFixture
{
    public static void main(String[] args) throws Exception
    {
        var backend = new InMemoryBackend();
        var flags = new RuntimeFeatureFlagStore();
        var queues = new RuntimeQueueStore();
        var leases = new RuntimeLeaseStore();
        var workflows = new RuntimeWorkflowStore();
        var logs = new RuntimeLogSink();
        var metrics = new RuntimeMetricSink();

        var tx = backend.begin();
        flags.registerDefinitionTx(
            tx,
            new FeatureFlag.Definition(
                "new_scheduler", FeatureFlag.Type.BOOL, new FeatureFlag.Value.BoolValue(true),
                FeatureFlag.ScopeKind.SYSTEM, Optional.empty(), Optional.empty(), Optional.empty()
            )
        );
        queues.registerDefinitionTx(
            tx, new Queue.RegisterQueueDefinitionRequest(new Queue.QueueDefinition(
                    "workflow", "launch", Duration.ofMinutes(1), 3, Optional.empty(), "{}"
                ))
        );
        var leaseDefinitionId = new Lease.LeaseDefinitionId("workflow_step", 1L);
        leases.registerDefinitionTx(
            tx, new Lease.LeaseDefinition(
                    leaseDefinitionId, "workflow/*", Duration.ofMinutes(1), Duration.ofSeconds(30),
                    Optional.empty(), true
                )
        );
        workflows.registerDefinitionTx(
            tx, new Workflow.RegisterWorkflowDefinitionRequest(new Workflow.WorkflowDefinition(
                    "ProvisionService", 1L, "validate_request", Duration.ofMinutes(1), false,
                    List.of(new Workflow.WorkflowStepDefinition(
                        "validate_request", Duration.ofSeconds(1), 3
                    )),
                    "{}"
                ))
        );
        logs.registerDefinitionTx(
            tx, new Log.Definition(
                    "launch_decision", Log.Level.INFO, "workflow.launch.decision", List.of()
                )
        );
        metrics.registerDefinitionTx(
            tx, new Metric.Definition(
                    "launch_attempts", Metric.Kind.COUNTER, "workflow_launch_attempts_total", "1",
                    List.of()
                )
        );
        backend.commit(tx);

        var flag = flags.evaluate(
            backend, new FeatureFlag.EvaluationRequest(
                         "new_scheduler",
                         new FeatureFlag.EvaluationContext(
                             Optional.empty(), Optional.empty(), Optional.empty(), Optional.empty()
                         )
                     )
        );
        if (flag.asBool().isEmpty() || !flag.asBool().get())
        {
            throw new IllegalStateException("feature flag evaluation failed");
        }

        var now = Instant.ofEpochSecond(100);
        var message = queues.enqueue(
            backend,
            new Queue.EnqueueMessageRequest("msg-1", "workflow", "launch", Optional.empty(), "{}")
        );
        if (!message.status().equals("Pending"))
        {
            throw new IllegalStateException("queue enqueue failed");
        }
        var claimed = queues.claim(
            backend, new Queue.ClaimMessageRequest(
                         "workflow", "launch", "worker-1", now, Duration.ofMinutes(1), 1
                     )
        );
        if (claimed.size() != 1)
        {
            throw new IllegalStateException("queue claim failed");
        }

        var lease = leases.acquire(
            backend,
            new Lease.LeaseAcquireRequest(leaseDefinitionId, "workflow/msg-1", "worker-1", now)
        );
        if (!lease.acquired() || lease.lease().isEmpty())
        {
            throw new IllegalStateException("lease acquire failed");
        }

        workflows.start(
            backend, new Workflow.StartWorkflowRequest(
                         "wf-1", "ProvisionService", 1L, "validate_request", "{}"
                     )
        );
        var steps = workflows.claimSteps(
            backend, new Workflow.ClaimWorkflowStepRequest(
                         "wf-1", "ProvisionService", 1L, "worker-1", now, Duration.ofMinutes(1), 1
                     )
        );
        if (steps.size() != 1)
        {
            throw new IllegalStateException("workflow claim failed");
        }
        workflows.keepAliveStep(
            backend, new Workflow.KeepAliveWorkflowStepRequest(
                         "wf-1", "worker-1", "validate_request", now, Duration.ofMinutes(1)
                     )
        );

        logs.emit(
            backend,
            new Log.Event("launch_decision", Log.Level.INFO, "workflow.launch.decision", Map.of())
        );
        metrics.record(
            backend, new Metric.Sample(
                         "launch_attempts", Metric.Kind.COUNTER, "workflow_launch_attempts_total",
                         1.0, "1", Map.of()
                     )
        );
        if (logs.inspectEvents(backend).size() != 1 || metrics.inspectSamples(backend).size() != 1)
        {
            throw new IllegalStateException("observability inspect failed");
        }

        var appTx = backend.begin();
        backend.put(appTx, "orders", "order-1", Json.object(Map.of("status", Json.string("new"))));
        backend.commit(appTx);
        var readTx = backend.begin();
        if (backend.query(readTx, "orders", new Backend.Query.KeyPrefix("order-")).size() != 1)
        {
            throw new IllegalStateException("backend query failed");
        }
        backend.commit(readTx);
    }
}
