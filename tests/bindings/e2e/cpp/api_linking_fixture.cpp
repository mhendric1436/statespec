#include "api/api_server.hpp"
#include "common/memory/backend.hpp"

#include <stdexcept>

class LinkingApiHandler final : public statespec_generated::api::IBusinessApiOperationHandler
{
  public:
    statespec_generated::api::ApiResponse
    handle_start_provision(const statespec_generated::api::ApiRequestContext& context) override
    {
        return record_request(context);
    }

    statespec_generated::api::ApiResponse handle_report_provision_ready(
        const statespec_generated::api::ApiRequestContext& context
    ) override
    {
        return record_request(context);
    }

  private:
    statespec_generated::api::ApiResponse
    record_request(const statespec_generated::api::ApiRequestContext& context)
    {
        backend_.ensure_collection(
            statespec::backend::CollectionDescriptor{
                "api_requests",
                {},
                {"id"},
                {},
                1,
            }
        );
        auto tx = backend_.begin();
        backend_.put(
            *tx, "api_requests", "request-1",
            statespec::backend::Json::object(
                {{"server", context.server_name}, {"api", context.api_name}}
            )
        );
        backend_.commit(*tx);

        return statespec_generated::api::ApiResponse{
            202,
            statespec::backend::Json::object({{"linked", true}}),
        };
    }

    statespec::backend::memory::InMemoryBackend backend_;
};

int main()
{
    const auto descriptor = statespec_generated::api::find_api_server("ProvisionApi");
    if (!descriptor.has_value())
    {
        throw std::runtime_error("ProvisionApi descriptor not found");
    }

    LinkingApiHandler handler;
    statespec::backend::memory::InMemoryBackend backend;
    statespec_generated::api::ApiServer server{*descriptor, backend, &handler};
    const auto response = server.handle(
        "ProvisionApi.StartProvision",
        statespec_generated::api::ApiRequestContext{
            "ProvisionApi",
            "StartProvision",
            "POST",
            "/v1/provision",
            statespec::backend::Json::object({{"tenant_id", "tenant-1"}}),
        }
    );

    if (!response.has_value() || response->status_code != 202)
    {
        throw std::runtime_error("generated API server did not dispatch to linked handler");
    }
}
