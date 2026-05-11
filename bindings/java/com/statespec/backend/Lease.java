package com.statespec.backend;

import com.statespec.backend.BackendModel.Backend;
import com.statespec.backend.BackendModel.BackendException;
import com.statespec.backend.BackendModel.Transaction;

import java.time.Duration;
import java.time.Instant;
import java.util.Optional;

public abstract class Lease {
    private final Backend backend;

    protected Lease(Backend backend) {
        this.backend = backend;
    }

    public record LeaseRecord(
        String resource,
        Optional<String> holder,
        Instant expiresAt,
        long fencingToken
    ) {}

    public record LeaseAcquireRequest(
        String resource,
        String holder,
        Instant now,
        Duration ttl
    ) {}

    public record LeaseRenewRequest(
        String resource,
        String holder,
        long fencingToken,
        Instant now,
        Duration ttl
    ) {}

    public record LeaseReleaseRequest(
        String resource,
        String holder,
        long fencingToken
    ) {}

    public record LeaseAcquireResult(
        boolean acquired,
        Optional<LeaseRecord> lease
    ) {}

    protected abstract LeaseAcquireResult acquire(
        Transaction tx,
        LeaseAcquireRequest request
    ) throws BackendException;

    protected abstract LeaseRecord renew(
        Transaction tx,
        LeaseRenewRequest request
    ) throws BackendException;

    protected abstract void release(
        Transaction tx,
        LeaseReleaseRequest request
    ) throws BackendException;

    protected abstract Optional<LeaseRecord> inspect(
        Transaction tx,
        String resource
    ) throws BackendException;

    public final LeaseAcquireResult acquire(LeaseAcquireRequest request) throws BackendException {
        Transaction tx = backend.begin();
        try {
            LeaseAcquireResult result = acquire(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final LeaseRecord renew(LeaseRenewRequest request) throws BackendException {
        Transaction tx = backend.begin();
        try {
            LeaseRecord result = renew(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final void release(LeaseReleaseRequest request) throws BackendException {
        Transaction tx = backend.begin();
        try {
            release(tx, request);
            backend.commit(tx);
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final Optional<LeaseRecord> inspect(String resource) throws BackendException {
        Transaction tx = backend.begin();
        try {
            Optional<LeaseRecord> result = inspect(tx, resource);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }
}
