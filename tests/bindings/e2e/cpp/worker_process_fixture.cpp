#include "common/memory/backend.hpp"
#include "common/runtime/workflow_store.hpp"
#include "worker/worker_process.hpp"
#include "worker/worker_runtime.hpp"
#include "worker/workflow_step_handlers.hpp"

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

class ProcessWorkflowStepHandler final : public statespec_generated::worker::IWorkflowStepHandler
{
  public:
    void handle_validate_request(
        const statespec_generated::worker::WorkflowStepHandlerContext& context
    ) override
    {
        if (context.workflow_name != "ProvisionService" || context.step_name != "validate_request")
        {
            throw std::runtime_error("unexpected workflow step");
        }
        handled_validate_request = true;
    }

    void handle_create_remote_service(
        const statespec_generated::worker::WorkflowStepHandlerContext&
    ) override
    {
        handled_create_remote_service = true;
    }

    void handle_wait_for_remote_service(
        const statespec_generated::worker::WorkflowStepHandlerContext&
    ) override
    {
        handled_wait_for_remote_service = true;
    }

    std::atomic_bool handled_validate_request = false;
    std::atomic_bool handled_create_remote_service = false;
    std::atomic_bool handled_wait_for_remote_service = false;
};

int main()
{
    statespec::backend::memory::InMemoryBackend backend;
    statespec_generated::worker::WorkerRuntime runtime{backend};
    ProcessWorkflowStepHandler handler;
    statespec_generated::worker::WorkerProcessConfig config;
    config.worker_poll_interval_ms = 1;
    statespec_generated::worker::WorkerProcess process{runtime, handler, config};

    if (process.join() == 0)
    {
        throw std::runtime_error("joining a WorkerProcess before start should fail");
    }
    process.request_stop();

    runtime.bootstrap();
    statespec::backend::runtime::RuntimeWorkflowStore workflows;
    workflows.start(
        backend, statespec::backend::StartWorkflowRequest{
                     "wf-process-1",
                     "ProvisionService",
                     1,
                     "validate_request",
                     statespec::backend::Json::object({}),
                 }
    );

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

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!handler.handled_validate_request.load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    if (!handler.handled_validate_request.load())
    {
        throw std::runtime_error("WorkerProcess did not execute a workflow step");
    }

    process.request_stop();
    if (process.join() != 0)
    {
        throw std::runtime_error("stopped WorkerProcess should join cleanly");
    }
    if (process.running())
    {
        throw std::runtime_error("joined WorkerProcess should not report running");
    }
}
