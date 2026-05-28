#pragma once

#include "../backend.hpp"
#include "entity_gc_repository.hpp"
#include "entity_gc_types.hpp"
#include "entity_gc_workers.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace statespec::backend::runtime
{

class DefaultEntityGcRepository final : public IEntityGcRepository
{
  public:
    std::vector<VersionedRecord> list_gc_eligibleTx(
        ITransaction& tx,
        const EntityGcEligibilityRequest& request
    ) override
    {
        std::vector<VersionedRecord> eligible;
        for (const auto& record : tx.query(request.descriptor.collection, Query::all()))
        {
            if (eligible.size() >= request.limit)
            {
                break;
            }
            const auto* status = record.document.find(request.descriptor.status_field);
            if (status == nullptr || !status->is_string())
            {
                continue;
            }
            if (is_terminal_gc_state(request.descriptor, status->as_string()))
            {
                eligible.push_back(record);
            }
        }
        return eligible;
    }

    void finalize_gcTx(
        ITransaction& tx,
        const EntityGcFinalizeRequest& request
    ) override
    {
        tx.erase(request.descriptor.collection, request.key);
    }

  private:
    static bool is_terminal_gc_state(
        const EntityGcDescriptor& descriptor,
        const std::string& status
    )
    {
        return std::any_of(
            descriptor.terminal_states.begin(), descriptor.terminal_states.end(),
            [&](const auto& terminal) { return terminal.state == status; }
        );
    }
};

template <typename Registrar>
void register_entity_gc_workers(
    Registrar& registrar,
    IBackend& backend,
    const std::vector<EntityGcDescriptor>& descriptors
)
{
    for (const auto& descriptor : descriptors)
    {
        auto repository = std::make_shared<DefaultEntityGcRepository>();
        auto worker =
            std::make_shared<EntityGcWorker>(descriptor, *repository, EntityGcWorkerConfig{});
        registrar.add_entity_gc_worker(
            [repository, worker, &backend](const std::string& now)
            {
                (void)repository;
                worker->run_once(backend, now);
            }
        );
    }
}

template <typename Registrar>
void register_entity_gc_workers(
    Registrar& registrar,
    IBackend& backend
)
{
    register_entity_gc_workers(registrar, backend, std::vector<EntityGcDescriptor>{});
}

} // namespace statespec::backend::runtime
