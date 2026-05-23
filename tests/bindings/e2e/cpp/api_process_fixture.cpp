#include "api/api_handler_registry.hpp"
#include "api/api_process.hpp"
#include "api/api_transport.hpp"
#include "common/memory/backend.hpp"

#include <stdexcept>
#include <thread>

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

    int status = 1;
    std::thread runner{[&] { status = process.run(); }};
    process.request_stop();
    runner.join();

    if (status != 0)
    {
        throw std::runtime_error("API process did not stop cleanly");
    }
}
