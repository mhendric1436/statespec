#pragma once

#include "../common/system_descriptors.hpp"

namespace statespec_generated::worker
{

inline std::vector<statespec::backend::QueueDefinition> queue_definitions()
{
    return ::statespec_generated::queue_definitions();
}

inline void register_queue_definitionsTx(
    statespec::backend::ITransaction& tx,
    statespec::backend::IQueueStore& queue_store
)
{
    ::statespec_generated::register_queue_definitionsTx(tx, queue_store);
}

} // namespace statespec_generated::worker
