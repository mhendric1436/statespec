#pragma once

#include "api_handlers.hpp"
#include "api_routes.hpp"

#include <optional>
#include <string_view>

namespace statespec_generated::api
{

inline std::optional<ApiRouteDescriptor> find_api_route(std::string_view route_name)
{
    for (const auto& route : api_route_descriptors())
    {
        if (route.name == route_name)
        {
            return route;
        }
    }
    return std::nullopt;
}

inline std::optional<ApiResponse> dispatch_api_route(
    IApiHandler& handler,
    std::string_view route_name,
    const ApiRequestContext& context
)
{
    if (!find_api_route(route_name).has_value())
    {
        return std::nullopt;
    }
    return handler.handle(context);
}

} // namespace statespec_generated::api
