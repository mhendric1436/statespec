#pragma once

#include "backend.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace statespec::backend
{

using Timestamp = std::chrono::system_clock::time_point;

struct LeaseDefinitionId
{
    std::string name;
    std::uint64_t version = 1;
};

struct LeaseDefinition
{
    LeaseDefinitionId id;
    std::string resource_pattern;
    std::chrono::seconds ttl;
    std::chrono::seconds renew_every;
    std::optional<std::chrono::seconds> max_ttl;
    bool fencing_token = true;
};

struct LeaseRegisterDefinitionResult
{
    bool registered_new = false;
    LeaseDefinition definition;
};

struct LeaseRecord
{
    LeaseDefinitionId definition_id;
    std::string resource;
    std::optional<std::string> holder;
    Timestamp expires_at;
    std::uint64_t fencing_token = 0;
};

struct LeaseAcquireRequest
{
    LeaseDefinitionId definition_id;
    std::string resource;
    std::string holder;
    Timestamp now;
};

struct LeaseRenewRequest
{
    LeaseDefinitionId definition_id;
    std::string resource;
    std::string holder;
    std::uint64_t fencing_token = 0;
    Timestamp now;
};

struct LeaseReleaseRequest
{
    LeaseDefinitionId definition_id;
    std::string resource;
    std::string holder;
    std::uint64_t fencing_token = 0;
};

struct LeaseInspectRequest
{
    LeaseDefinitionId definition_id;
    std::string resource;
};

struct LeaseAcquireResult
{
    bool acquired = false;
    std::optional<LeaseRecord> lease;
};

class ILeaseStore
{
  public:
    virtual ~ILeaseStore() = default;

    virtual LeaseRegisterDefinitionResult register_definition(
        IBackend& backend,
        const LeaseDefinition& definition
    ) = 0;

    virtual LeaseRegisterDefinitionResult register_definitionTx(
        ITransaction& tx,
        const LeaseDefinition& definition
    ) = 0;

    virtual std::optional<LeaseDefinition> inspect_definition(
        IBackend& backend,
        const LeaseDefinitionId& definition_id
    ) = 0;

    virtual std::optional<LeaseDefinition> inspect_definitionTx(
        ITransaction& tx,
        const LeaseDefinitionId& definition_id
    ) = 0;

    virtual LeaseAcquireResult acquire(
        IBackend& backend,
        const LeaseAcquireRequest& request
    ) = 0;

    virtual LeaseAcquireResult acquireTx(
        ITransaction& tx,
        const LeaseAcquireRequest& request
    ) = 0;

    virtual LeaseRecord renew(
        IBackend& backend,
        const LeaseRenewRequest& request
    ) = 0;

    virtual LeaseRecord renewTx(
        ITransaction& tx,
        const LeaseRenewRequest& request
    ) = 0;

    virtual void release(
        IBackend& backend,
        const LeaseReleaseRequest& request
    ) = 0;

    virtual void releaseTx(
        ITransaction& tx,
        const LeaseReleaseRequest& request
    ) = 0;

    virtual std::optional<LeaseRecord> inspect(
        IBackend& backend,
        const LeaseInspectRequest& request
    ) = 0;

    virtual std::optional<LeaseRecord> inspectTx(
        ITransaction& tx,
        const LeaseInspectRequest& request
    ) = 0;
};

} // namespace statespec::backend
