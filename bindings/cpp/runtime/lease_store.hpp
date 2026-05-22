#pragma once

#include "backend.hpp"
#include "codec.hpp"

namespace statespec::backend::runtime
{

class RuntimeLeaseStore : public ILeaseStore
{
  public:
    LeaseRegisterDefinitionResult register_definition(
        IBackend& backend,
        const LeaseDefinition& definition
    ) override
    {
        ensure_collections(backend);
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
        const auto existing = inspect_definitionTx(tx, definition.id);
        tx.put(
            kDefinitionsCollection, definition_key(definition.id),
            detail::lease_definition_to_json(definition)
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
        const auto record = tx.get(kDefinitionsCollection, definition_key(definition_id));
        if (!record.has_value())
        {
            return std::nullopt;
        }
        return detail::lease_definition_from_json(record->document);
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
        const auto definition = require_definition(tx, request.definition_id);
        const auto existing =
            inspectTx(tx, LeaseInspectRequest{request.definition_id, request.resource});
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
        tx.put(
            kLeasesCollection, lease_key(request.definition_id, request.resource),
            detail::lease_record_to_json(lease)
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
        const auto definition = require_definition(tx, request.definition_id);
        auto lease = require_lease(tx, request.definition_id, request.resource);
        require_holder(lease, request.holder, request.fencing_token);
        lease.expires_at = request.now + definition.ttl;
        tx.put(
            kLeasesCollection, lease_key(request.definition_id, request.resource),
            detail::lease_record_to_json(lease)
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
        auto lease = require_lease(tx, request.definition_id, request.resource);
        require_holder(lease, request.holder, request.fencing_token);
        tx.erase(kLeasesCollection, lease_key(request.definition_id, request.resource));
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
        const auto record =
            tx.get(kLeasesCollection, lease_key(request.definition_id, request.resource));
        if (!record.has_value())
        {
            return std::nullopt;
        }
        return detail::lease_record_from_json(record->document);
    }

  private:
    static constexpr const char* kDefinitionsCollection = runtime_collections::LeaseDefinitions;
    static constexpr const char* kLeasesCollection = runtime_collections::Leases;

    static void ensure_collections(IBackend& backend)
    {
        backend.ensure_collections(
            {CollectionDescriptor{
                 .name = kDefinitionsCollection,
                 .key_fields = {std::string{runtime_key_fields::LeaseDefinition}}
             },
             CollectionDescriptor{
                 .name = kLeasesCollection, .key_fields = {std::string{runtime_key_fields::Lease}}
             }}
        );
    }

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
        RuntimeLeaseStore store;
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
        RuntimeLeaseStore store;
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

} // namespace statespec::backend::runtime
