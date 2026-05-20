package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.Lease;
import java.util.Optional;

public final class InMemoryLeaseStore implements Lease
{
    @Override
    public LeaseRegisterDefinitionResult registerDefinition(
        Backend backend,
        LeaseDefinition definition
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = registerDefinitionTx(tx, definition);
            backend.commit(tx);
            return result;
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public LeaseRegisterDefinitionResult registerDefinitionTx(
        Backend.Transaction tx,
        LeaseDefinition definition
    ) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        var existing = inspectDefinitionTx(tx, definition.id());
        memoryTx.leaseDefinitionPuts.put(leaseDefinitionKey(definition.id()), definition);
        return new LeaseRegisterDefinitionResult(existing.isEmpty(), definition);
    }

    @Override
    public Optional<LeaseDefinition> inspectDefinition(
        Backend backend,
        LeaseDefinitionId definitionId
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        var result = inspectDefinitionTx(tx, definitionId);
        backend.commit(tx);
        return result;
    }

    @Override
    public Optional<LeaseDefinition> inspectDefinitionTx(
        Backend.Transaction tx,
        LeaseDefinitionId definitionId
    ) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        var key = leaseDefinitionKey(definitionId);
        if (memoryTx.leaseDefinitionPuts.containsKey(key))
        {
            return Optional.of(memoryTx.leaseDefinitionPuts.get(key));
        }
        synchronized (memoryTx.state)
        {
            return Optional.ofNullable(memoryTx.state.leaseDefinitions.get(key));
        }
    }

    @Override
    public LeaseAcquireResult acquire(
        Backend backend,
        LeaseAcquireRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = acquireTx(tx, request);
            backend.commit(tx);
            return result;
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public LeaseAcquireResult acquireTx(
        Backend.Transaction tx,
        LeaseAcquireRequest request
    ) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        var definition = inspectDefinitionTx(tx, request.definitionId());
        if (definition.isEmpty())
        {
            throw new Backend.BackendException("unknown lease definition");
        }
        var existing =
            inspectTx(tx, new LeaseInspectRequest(request.definitionId(), request.resource()));
        if (existing.isPresent() && existing.get().holder().isPresent() &&
            !existing.get().holder().get().equals(request.holder()) &&
            existing.get().expiresAt().isAfter(request.now()))
        {
            return new LeaseAcquireResult(false, existing);
        }
        long token = existing.map(LeaseRecord::fencingToken).orElse(0L);
        if (definition.get().fencingToken())
        {
            token++;
        }
        var record = new LeaseRecord(
            request.definitionId(), request.resource(), Optional.of(request.holder()),
            request.now().plus(definition.get().ttl()), token
        );
        var key = leaseKey(request.definitionId(), request.resource());
        memoryTx.leasePuts.put(key, record);
        memoryTx.leaseErases.remove(key);
        return new LeaseAcquireResult(true, Optional.of(record));
    }

    @Override
    public LeaseRecord renew(
        Backend backend,
        LeaseRenewRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = renewTx(tx, request);
            backend.commit(tx);
            return result;
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public LeaseRecord renewTx(
        Backend.Transaction tx,
        LeaseRenewRequest request
    ) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        var definition = inspectDefinitionTx(tx, request.definitionId());
        if (definition.isEmpty())
        {
            throw new Backend.BackendException("unknown lease definition");
        }
        var record = requireLease(tx, request.definitionId(), request.resource());
        requireHolder(record, request.holder(), request.fencingToken());
        var updated = new LeaseRecord(
            record.definitionId(), record.resource(), record.holder(),
            request.now().plus(definition.get().ttl()), record.fencingToken()
        );
        memoryTx.leasePuts.put(leaseKey(request.definitionId(), request.resource()), updated);
        return updated;
    }

    @Override
    public void release(
        Backend backend,
        LeaseReleaseRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            releaseTx(tx, request);
            backend.commit(tx);
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public void releaseTx(
        Backend.Transaction tx,
        LeaseReleaseRequest request
    ) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        var record = requireLease(tx, request.definitionId(), request.resource());
        requireHolder(record, request.holder(), request.fencingToken());
        var key = leaseKey(request.definitionId(), request.resource());
        memoryTx.leasePuts.remove(key);
        memoryTx.leaseErases.add(key);
    }

    @Override
    public Optional<LeaseRecord> inspect(
        Backend backend,
        LeaseInspectRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        var result = inspectTx(tx, request);
        backend.commit(tx);
        return result;
    }

    @Override
    public Optional<LeaseRecord> inspectTx(
        Backend.Transaction tx,
        LeaseInspectRequest request
    ) throws Backend.BackendException
    {
        var memoryTx = InMemoryTransaction.require(tx);
        var key = leaseKey(request.definitionId(), request.resource());
        if (memoryTx.leaseErases.contains(key))
        {
            return Optional.empty();
        }
        if (memoryTx.leasePuts.containsKey(key))
        {
            return Optional.of(memoryTx.leasePuts.get(key));
        }
        synchronized (memoryTx.state)
        {
            return Optional.ofNullable(memoryTx.state.leases.get(key));
        }
    }

    private LeaseRecord requireLease(
        Backend.Transaction tx,
        LeaseDefinitionId definitionId,
        String resource
    ) throws Backend.BackendException
    {
        return inspectTx(tx, new LeaseInspectRequest(definitionId, resource))
            .orElseThrow(() -> new Backend.BackendException("unknown lease"));
    }

    private static String leaseDefinitionKey(LeaseDefinitionId id)
    {
        return InMemoryTransaction.definitionKey(id.name(), id.version());
    }

    private static String leaseKey(
        LeaseDefinitionId id,
        String resource
    )
    {
        return InMemoryTransaction.definitionKey(id.name(), id.version(), resource);
    }

    private static void requireHolder(
        LeaseRecord record,
        String holder,
        long fencingToken
    ) throws Backend.BackendException
    {
        if (record.holder().isEmpty() || !record.holder().get().equals(holder) ||
            record.fencingToken() != fencingToken)
        {
            throw new Backend.ConflictException(
                Backend.ConflictKind.LEASE_CONFLICT, "lease is not held by caller"
            );
        }
    }
}
