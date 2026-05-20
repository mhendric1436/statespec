#pragma once

#include "../common/system_descriptors.hpp"

namespace statespec_generated::worker
{

using WorkerDescriptor = ::statespec_generated::WorkerDescriptor;

inline std::vector<WorkerDescriptor> worker_descriptors()
{
    return ::statespec_generated::worker_descriptors();
}

} // namespace statespec_generated::worker
