#pragma once

#include "backend.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace statespec::backend
{

using Timestamp = std::chrono::system_clock::time_point;

struct LeaseRecord
{
    std::string resource;
    std::optional<std::string> holder;
    Timestamp expires_at;
    std::uint64_t fencing_token = 0;
};

struct LeaseAcquireRequest
{
    std::string resource;
    std::string holder;
    Timestamp now;
    std::chrono::seconds ttl;
};

struct LeaseRenewRequest
{
    std::string resource;
    std::string holder;
    std::uint64_t fencing_token = 0;
    Timestamp now;
    std::chrono::seconds ttl;
};

struct LeaseReleaseRequest
{
    std::string resource;
    std::string holder;
    std::uint64_t fencing_token = 0;
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

    virtual LeaseAcquireResult acquire(
        IBackend& backend,
        const LeaseAcquireRequest& request
    ) = 0;

    virtual LeaseRecord renew(
        IBackend& backend,
        const LeaseRenewRequest& request
    ) = 0;

    virtual void release(
        IBackend& backend,
        const LeaseReleaseRequest& request
    ) = 0;

    virtual std::optional<LeaseRecord> inspect(
        IBackend& backend,
        const std::string& resource
    ) = 0;
};

} // namespace statespec::backend
