#include "api/api_handler_registry.hpp"
#include "api/api_process.hpp"
#include "api/api_transport.hpp"
#include "common/memory/backend.hpp"

#include <stdexcept>

int main()
{
    statespec::backend::memory::InMemoryBackend backend;
    statespec_generated::api::DefaultApiHandler handler{backend};
    statespec_generated::api::LocalBlockingApiTransport transport;
    auto config = statespec_generated::api::ApiProcessConfig::all_servers();
    if (config.server_names.size() != 2)
    {
        throw std::runtime_error("expected two generated API server descriptors");
    }

    statespec_generated::api::ApiProcess process{config, handler, backend, transport};
    if (process.applications().size() != 2)
    {
        throw std::runtime_error("expected two generated API applications");
    }

    statespec_generated::api::LocalBlockingApiTransport not_started_transport;
    statespec_generated::api::ApiProcess not_started{
        config,
        handler,
        backend,
        not_started_transport
    };
    not_started.request_stop();
    if (not_started.join() == 0)
    {
        throw std::runtime_error("join before start should fail");
    }

    if (process.start() != 0)
    {
        throw std::runtime_error("API process did not start cleanly");
    }
    if (!process.running())
    {
        throw std::runtime_error("API process did not report running");
    }
    if (process.start() == 0)
    {
        throw std::runtime_error("API process allowed double start");
    }
    process.request_stop();
    const auto status = process.join();

    if (status != 0)
    {
        throw std::runtime_error("API process did not stop cleanly");
    }
    if (process.running())
    {
        throw std::runtime_error("API process still reports running after join");
    }
}
