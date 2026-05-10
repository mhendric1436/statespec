package com.statespec.backend;

import com.statespec.backend.BackendModel.BackendException;
import com.statespec.backend.BackendModel.LeaseRecord;
import com.statespec.backend.BackendModel.Transaction;

import java.time.Duration;
import java.time.Instant;
import java.util.Optional;

public interface LeaseStore {
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
        Transaction tx,
        LeaseAcquireRequest request
    ) throws BackendException;

    LeaseRecord renew(
        Transaction tx,
        LeaseRenewRequest request
    ) throws BackendException;

    void release(
        Transaction tx,
        LeaseReleaseRequest request
    ) throws BackendException;

    Optional<LeaseRecord> inspect(
        Transaction tx,
        String resource
    ) throws BackendException;
}
