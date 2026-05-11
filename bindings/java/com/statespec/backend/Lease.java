package com.statespec.backend;

import com.statespec.backend.BackendModel.Backend;
import com.statespec.backend.BackendModel.BackendException;
import com.statespec.backend.BackendModel.Transaction;

import java.time.Duration;
import java.time.Instant;
import java.util.Optional;

public interface Lease {
    record LeaseRecord(
        String resource,
        Optional<String> holder,
        Instant expiresAt,
        long fencingToken
    ) {}

    record LeaseAcquireRequest(
        String resource,
        String holder,
        Instant now,
        Duration ttl
    ) {}

    record LeaseRenewRequest(
        String resource,
        String holder,
        long fencingToken,
        Instant now,
        Duration ttl
    ) {}

    record LeaseReleaseRequest(
        String resource,
        String holder,
        long fencingToken
    ) {}

    record LeaseAcquireResult(
        boolean acquired,
        Optional<LeaseRecord> lease
    ) {}

    LeaseAcquireResult acquire(
        Backend backend,
        LeaseAcquireRequest request
    ) throws BackendException;

    LeaseAcquireResult acquireTx(
        Transaction tx,
        LeaseAcquireRequest request
    ) throws BackendException;

    LeaseRecord renew(
        Backend backend,
        LeaseRenewRequest request
    ) throws BackendException;

    LeaseRecord renewTx(
        Transaction tx,
        LeaseRenewRequest request
    ) throws BackendException;

    void release(
        Backend backend,
        LeaseReleaseRequest request
    ) throws BackendException;

    void releaseTx(
        Transaction tx,
        LeaseReleaseRequest request
    ) throws BackendException;

    Optional<LeaseRecord> inspect(
        Backend backend,
        String resource
    ) throws BackendException;

    Optional<LeaseRecord> inspectTx(
        Transaction tx,
        String resource
    ) throws BackendException;
}
