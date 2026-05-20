package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.FeatureFlag;
import com.statespec.backend.Json;
import com.statespec.backend.Lease;
import com.statespec.backend.Log;
import com.statespec.backend.Metric;
import com.statespec.backend.Queue;
import com.statespec.backend.Workflow;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public final class InMemoryTransaction implements Backend.Transaction
{
    static final class State
    {
        final Map<String, Map<String, Backend.VersionedRecord>> records = new HashMap<>();
        final Map<String, Long> versions = new HashMap<>();
        final Map<String, Backend.CollectionDescriptor> collections = new HashMap<>();
        final Map<String, FeatureFlag.Definition> featureFlagDefinitions = new HashMap<>();
        final Map<String, FeatureFlag.Value> featureFlagValues = new HashMap<>();
        final Map<String, Queue.QueueDefinition> queueDefinitions = new HashMap<>();
        final Map<String, Queue.QueueMessageRecord> queueMessages = new HashMap<>();
        final Map<String, String> queueIdempotencyKeys = new HashMap<>();
        final Map<String, Lease.LeaseDefinition> leaseDefinitions = new HashMap<>();
        final Map<String, Lease.LeaseRecord> leases = new HashMap<>();
        final Map<String, Workflow.WorkflowDefinition> workflowDefinitions = new HashMap<>();
        final Map<String, Workflow.WorkflowExecutionRecord> workflowExecutions = new HashMap<>();
        final Map<String, Log.Definition> logDefinitions = new HashMap<>();
        final List<Log.Event> logEvents = new ArrayList<>();
        final Map<String, Metric.Definition> metricDefinitions = new HashMap<>();
        final List<Metric.Sample> metricSamples = new ArrayList<>();
    }

    final State state;
    boolean open = true;
    final Map<String, Long> readVersions = new HashMap<>();
    final Map<String, Backend.VersionedRecord> puts = new HashMap<>();
    final Set<String> erases = new HashSet<>();
    final Map<String, FeatureFlag.Definition> featureFlagDefinitionPuts = new HashMap<>();
    final Map<String, FeatureFlag.Value> featureFlagValuePuts = new HashMap<>();
    final Map<String, Queue.QueueDefinition> queueDefinitionPuts = new HashMap<>();
    final Map<String, Queue.QueueMessageRecord> queueMessagePuts = new HashMap<>();
    final Map<String, String> queueIdempotencyPuts = new HashMap<>();
    final Map<String, Lease.LeaseDefinition> leaseDefinitionPuts = new HashMap<>();
    final Map<String, Lease.LeaseRecord> leasePuts = new HashMap<>();
    final Set<String> leaseErases = new HashSet<>();
    final Map<String, Workflow.WorkflowDefinition> workflowDefinitionPuts = new HashMap<>();
    final Map<String, Workflow.WorkflowExecutionRecord> workflowExecutionPuts = new HashMap<>();
    final Map<String, Log.Definition> logDefinitionPuts = new HashMap<>();
    final List<Log.Event> logEventAppends = new ArrayList<>();
    final Map<String, Metric.Definition> metricDefinitionPuts = new HashMap<>();
    final List<Metric.Sample> metricSampleAppends = new ArrayList<>();

    InMemoryTransaction(State state)
    {
        this.state = state;
    }

    @Override public boolean isOpen()
    {
        return open;
    }

    @Override public void abort()
    {
        open = false;
        readVersions.clear();
        puts.clear();
        erases.clear();
        featureFlagDefinitionPuts.clear();
        featureFlagValuePuts.clear();
        queueDefinitionPuts.clear();
        queueMessagePuts.clear();
        queueIdempotencyPuts.clear();
        leaseDefinitionPuts.clear();
        leasePuts.clear();
        leaseErases.clear();
        workflowDefinitionPuts.clear();
        workflowExecutionPuts.clear();
        logDefinitionPuts.clear();
        logEventAppends.clear();
        metricDefinitionPuts.clear();
        metricSampleAppends.clear();
    }

    static InMemoryTransaction require(Backend.Transaction tx) throws Backend.BackendException
    {
        if (!(tx instanceof InMemoryTransaction memoryTx) || !memoryTx.isOpen())
        {
            throw new Backend.BackendException("expected open in-memory transaction");
        }
        return memoryTx;
    }

    static String recordVersionKey(
        String collection,
        String key
    )
    {
        return "record:" + collection + ":" + key;
    }

    static long versionOrZero(
        Map<String,
            Long> versions,
        String key
    )
    {
        return versions.getOrDefault(key, 0L);
    }

    static String definitionKey(Object... parts)
    {
        StringBuilder key = new StringBuilder();
        for (int index = 0; index < parts.length; index++)
        {
            if (index > 0)
            {
                key.append(':');
            }
            key.append(parts[index]);
        }
        return key.toString();
    }

    static Json findJsonPath(
        Json document,
        String path
    )
    {
        Json current = document;
        for (String segment : path.split("\\."))
        {
            var next = current.find(segment);
            if (next.isEmpty())
            {
                return null;
            }
            current = next.get();
        }
        return current;
    }
}
