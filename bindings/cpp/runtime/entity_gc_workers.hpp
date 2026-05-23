#pragma once

#include "../backend.hpp"
#include "entity_gc_descriptors.hpp"
#include "entity_gc_repository.hpp"

#include <cstddef>
#include <utility>

namespace statespec::backend::runtime
{

struct EntityGcWorkerConfig
{
    std::size_t batch_limit = 100;
};

struct EntityGcRunResult
{
    std::size_t scanned = 0;
    std::size_t finalized = 0;
};

class EntityGcWorker
{
  public:
    EntityGcWorker(
        EntityGcDescriptor descriptor,
        IEntityGcRepository& repository,
        EntityGcWorkerConfig config = {}
    )
        : descriptor_(std::move(descriptor)),
          repository_(repository),
          config_(config)
    {
    }

    EntityGcRunResult run_once(
        IBackend& backend,
        const std::string& now
    )
    {
        auto tx = backend.begin();
        try
        {
            auto result = run_onceTx(*tx, now);
            backend.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    EntityGcRunResult run_onceTx(
        ITransaction& tx,
        const std::string& now
    )
    {
        auto records = repository_.list_gc_eligibleTx(
            tx, EntityGcEligibilityRequest{descriptor_, now, config_.batch_limit}
        );
        EntityGcRunResult result{records.size(), 0};
        for (const auto& record : records)
        {
            repository_.finalize_gcTx(tx, EntityGcFinalizeRequest{descriptor_, record.key, now});
            ++result.finalized;
        }
        return result;
    }

    const EntityGcDescriptor& descriptor() const
    {
        return descriptor_;
    }

  private:
    EntityGcDescriptor descriptor_;
    IEntityGcRepository& repository_;
    EntityGcWorkerConfig config_;
};

} // namespace statespec::backend::runtime
