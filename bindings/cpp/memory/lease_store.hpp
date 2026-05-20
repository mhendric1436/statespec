#pragma once

#include "backend.hpp"

namespace statespec::backend::memory
{

class InMemoryLeaseStore : public ILeaseStore
{
  public:
    LeaseRegisterDefinitionResult register_definition(
        IBackend& backend,
        const LeaseDefinition& definition
    ) override
    {
        auto tx = backend.begin();
        auto result = register_definitionTx(*tx, definition);
        backend.commit(*tx);
        return result;
    }

    LeaseRegisterDefinitionResult register_definitionTx(
        ITransaction& tx,
        const LeaseDefinition& definition
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto existing = inspect_definitionTx(memory_tx, definition.id);
        memory_tx.lease_definition_puts().insert_or_assign(
            definition_key(definition.id), definition
        );
        return LeaseRegisterDefinitionResult{!existing.has_value(), definition};
    }

    std::optional<LeaseDefinition> inspect_definition(
        IBackend& backend,
        const LeaseDefinitionId& definition_id
    ) override
    {
        auto tx = backend.begin();
        auto result = inspect_definitionTx(*tx, definition_id);
        backend.commit(*tx);
        return result;
    }

    std::optional<LeaseDefinition> inspect_definitionTx(
        ITransaction& tx,
        const LeaseDefinitionId& definition_id
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto key = definition_key(definition_id);
        if (const auto staged = memory_tx.lease_definition_puts().find(key);
            staged != memory_tx.lease_definition_puts().end())
        {
            return staged->second;
        }
        std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
        if (const auto iter = memory_tx.state().lease_definitions.find(key);
            iter != memory_tx.state().lease_definitions.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    LeaseAcquireResult acquire(
        IBackend& backend,
        const LeaseAcquireRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = acquireTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    LeaseAcquireResult acquireTx(
        ITransaction& tx,
        const LeaseAcquireRequest& request
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto definition = require_definition(memory_tx, request.definition_id);
        const auto existing =
            inspectTx(memory_tx, LeaseInspectRequest{request.definition_id, request.resource});
        if (existing.has_value() && existing->holder.has_value() &&
            existing->expires_at > request.now)
        {
            return LeaseAcquireResult{false, existing};
        }

        LeaseRecord lease;
        lease.definition_id = request.definition_id;
        lease.resource = request.resource;
        lease.holder = request.holder;
        lease.expires_at = request.now + definition.ttl;
        lease.fencing_token = existing.has_value() ? existing->fencing_token + 1 : 1;
        memory_tx.lease_puts().insert_or_assign(
            lease_key(request.definition_id, request.resource), lease
        );
        return LeaseAcquireResult{true, lease};
    }

    LeaseRecord renew(
        IBackend& backend,
        const LeaseRenewRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = renewTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    LeaseRecord renewTx(
        ITransaction& tx,
        const LeaseRenewRequest& request
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto definition = require_definition(memory_tx, request.definition_id);
        auto lease = require_lease(memory_tx, request.definition_id, request.resource);
        require_holder(lease, request.holder, request.fencing_token);
        lease.expires_at = request.now + definition.ttl;
        memory_tx.lease_puts().insert_or_assign(
            lease_key(request.definition_id, request.resource), lease
        );
        return lease;
    }

    void release(
        IBackend& backend,
        const LeaseReleaseRequest& request
    ) override
    {
        auto tx = backend.begin();
        releaseTx(*tx, request);
        backend.commit(*tx);
    }

    void releaseTx(
        ITransaction& tx,
        const LeaseReleaseRequest& request
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        auto lease = require_lease(memory_tx, request.definition_id, request.resource);
        require_holder(lease, request.holder, request.fencing_token);
        memory_tx.lease_erases().insert(lease_key(request.definition_id, request.resource));
    }

    std::optional<LeaseRecord> inspect(
        IBackend& backend,
        const LeaseInspectRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = inspectTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    std::optional<LeaseRecord> inspectTx(
        ITransaction& tx,
        const LeaseInspectRequest& request
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto key = lease_key(request.definition_id, request.resource);
        if (memory_tx.lease_erases().find(key) != memory_tx.lease_erases().end())
        {
            return std::nullopt;
        }
        if (const auto staged = memory_tx.lease_puts().find(key);
            staged != memory_tx.lease_puts().end())
        {
            return staged->second;
        }
        std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
        if (const auto iter = memory_tx.state().leases.find(key);
            iter != memory_tx.state().leases.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

  private:
    static std::string definition_key(const LeaseDefinitionId& id)
    {
        return id.name + ":" + std::to_string(id.version);
    }

    static std::string lease_key(
        const LeaseDefinitionId& id,
        const std::string& resource
    )
    {
        return definition_key(id) + ":" + resource;
    }

    static LeaseDefinition require_definition(
        ITransaction& tx,
        const LeaseDefinitionId& id
    )
    {
        InMemoryLeaseStore store;
        auto definition = store.inspect_definitionTx(tx, id);
        if (!definition.has_value())
        {
            throw BackendError("unknown lease definition: " + id.name);
        }
        return *definition;
    }

    static LeaseRecord require_lease(
        ITransaction& tx,
        const LeaseDefinitionId& id,
        const std::string& resource
    )
    {
        InMemoryLeaseStore store;
        auto lease = store.inspectTx(tx, LeaseInspectRequest{id, resource});
        if (!lease.has_value())
        {
            throw BackendError("unknown lease: " + resource);
        }
        return *lease;
    }

    static void require_holder(
        const LeaseRecord& lease,
        const std::string& holder,
        std::uint64_t fencing_token
    )
    {
        if (!lease.holder.has_value() || *lease.holder != holder ||
            (fencing_token != 0 && lease.fencing_token != fencing_token))
        {
            throw ConflictError(ConflictKind::LeaseConflict, "lease is not held by caller");
        }
    }
};

} // namespace statespec::backend::memory
