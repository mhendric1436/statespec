package com.statespec.backend;

import com.statespec.backend.Backend.BackendException;
import com.statespec.backend.Backend.Transaction;

import java.time.Duration;
import java.time.Instant;
import java.util.Optional;

public interface Lease {
    record LeaseDefinitionId(
        String name,
        long version
    ) {}

    record LeaseDefinition(
        LeaseDefinitionId id,
        String resourcePattern,
        Duration ttl,
        Duration renewEvery,
        Optional<Duration> maxTtl,
        boolean fencingToken
    ) {}

    record LeaseRegisterDefinitionResult(
        boolean registeredNew,
        LeaseDefinition definition
    ) {}

    record LeaseRecord(
        LeaseDefinitionId definitionId,
        String resource,
        Optional<String> holder,
        Instant expiresAt,
        long fencingToken
    ) {}

    record LeaseAcquireRequest(
        LeaseDefinitionId definitionId,
        String resource,
        String holder,
        Instant now
    ) {}

    record LeaseRenewRequest(
        LeaseDefinitionId definitionId,
        String resource,
        String holder,
        long fencingToken,
        Instant now
    ) {}

    record LeaseReleaseRequest(
        LeaseDefinitionId definitionId,
        String resource,
        String holder,
        long fencingToken
    ) {}

    record LeaseInspectRequest(
        LeaseDefinitionId definitionId,
        String resource
    ) {}

    record LeaseAcquireResult(
        boolean acquired,
        Optional<LeaseRecord> lease
    ) {}

    LeaseRegisterDefinitionResult registerDefinition(
        Backend backend,
        LeaseDefinition definition
    ) throws BackendException;

    LeaseRegisterDefinitionResult registerDefinitionTx(
        Transaction tx,
        LeaseDefinition definition
    ) throws BackendException;

    Optional<LeaseDefinition> inspectDefinition(
        Backend backend,
        LeaseDefinitionId definitionId
    ) throws BackendException;

    Optional<LeaseDefinition> inspectDefinitionTx(
        Transaction tx,
        LeaseDefinitionId definitionId
    ) throws BackendException;

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
        LeaseInspectRequest request
    ) throws BackendException;

    Optional<LeaseRecord> inspectTx(
        Transaction tx,
        LeaseInspectRequest request
    ) throws BackendException;
}
