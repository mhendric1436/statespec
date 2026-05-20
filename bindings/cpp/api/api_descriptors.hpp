#pragma once

#include "../common/system_descriptors.hpp"

namespace statespec_generated::api
{

using ApiDescriptor = ::statespec_generated::ApiDescriptor;
using ApiServerDescriptor = ::statespec_generated::ApiServerDescriptor;

inline std::vector<ApiDescriptor> api_descriptors()
{
    return ::statespec_generated::api_descriptors();
}

inline std::vector<ApiServerDescriptor> api_server_descriptors()
{
    return ::statespec_generated::api_server_descriptors();
}

} // namespace statespec_generated::api
