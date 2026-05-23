#pragma once

#include "../backend.hpp"
#include "entity_gc_descriptors.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace statespec::backend::runtime
{

struct EntityGcEligibilityRequest
{
    EntityGcDescriptor descriptor;
    std::string now;
    std::size_t limit = 100;
};

struct EntityGcFinalizeRequest
{
    EntityGcDescriptor descriptor;
    Key key;
    std::string now;
};

class IEntityGcRepository
{
  public:
    virtual ~IEntityGcRepository() = default;

    virtual std::vector<VersionedRecord> list_gc_eligibleTx(
        ITransaction& tx,
        const EntityGcEligibilityRequest& request
    ) = 0;

    virtual void finalize_gcTx(
        ITransaction& tx,
        const EntityGcFinalizeRequest& request
    ) = 0;
};

} // namespace statespec::backend::runtime
