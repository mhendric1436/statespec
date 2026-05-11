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
        ITransaction& tx,
        const LeaseAcquireRequest& request
    ) = 0;

    virtual LeaseRecord renew(
        ITransaction& tx,
        const LeaseRenewRequest& request
    ) = 0;

    virtual void release(
        ITransaction& tx,
        const LeaseReleaseRequest& request
    ) = 0;

    virtual std::optional<LeaseRecord> inspect(
        ITransaction& tx,
        const std::string& resource
    ) = 0;
};

class Lease : public ILeaseStore
{
  public:
    explicit Lease(IBackend& backend)
        : backend_(backend)
    {
    }

    LeaseAcquireResult acquire(const LeaseAcquireRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = acquire(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    LeaseRecord renew(const LeaseRenewRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = renew(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    void release(const LeaseReleaseRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            release(*tx, request);
            backend_.commit(*tx);
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    std::optional<LeaseRecord> inspect(const std::string& resource)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = inspect(*tx, resource);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

  private:
    IBackend& backend_;
};

} // namespace statespec::backend
