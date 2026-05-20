#pragma once

#include "api_descriptors.hpp"
#include "api_dispatcher.hpp"

#include <optional>
#include <string_view>

#include <utility>

namespace statespec_generated::api
{

inline std::optional<ApiServerDescriptor> find_api_server(std::string_view server_name)
{
    for (const auto& server : api_server_descriptors())
    {
        if (server.name == server_name)
        {
            return server;
        }
    }
    return std::nullopt;
}

class ApiServer
{
  public:
    ApiServer(
        ApiServerDescriptor descriptor,
        IApiHandler& handler
    )
        : descriptor_(std::move(descriptor)),
          handler_(handler)
    {
    }

    const ApiServerDescriptor& descriptor() const
    {
        return descriptor_;
    }

    std::optional<ApiResponse> handle(
        std::string_view route_name,
        const ApiRequestContext& context
    )
    {
        const auto route = find_api_route(route_name);
        if (!route.has_value() || route->server_name != descriptor_.name)
        {
            return std::nullopt;
        }
        return dispatch_api_route(handler_, route_name, context);
    }

  private:
    ApiServerDescriptor descriptor_;
    IApiHandler& handler_;
};

} // namespace statespec_generated::api
