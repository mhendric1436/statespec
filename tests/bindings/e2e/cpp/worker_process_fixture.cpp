#include "common/memory/backend.hpp"
#include "worker/worker_process.hpp"
#include "worker/worker_runtime.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

int main()
{
    statespec::backend::memory::InMemoryBackend backend;
    statespec_generated::worker::WorkerRuntime runtime{backend};
    statespec_generated::worker::WorkerProcess process{runtime};

    if (process.join() == 0)
    {
        throw std::runtime_error("joining a WorkerProcess before start should fail");
    }
    process.request_stop();

    if (process.start() != 0)
    {
        throw std::runtime_error("starting WorkerProcess failed");
    }
    if (process.start() == 0)
    {
        throw std::runtime_error("starting WorkerProcess twice should fail");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    if (!process.running())
    {
        throw std::runtime_error("started WorkerProcess should report running");
    }

    process.request_stop();
    if (process.join() != 0)
    {
        throw std::runtime_error("stopped WorkerProcess should join cleanly");
    }
}
