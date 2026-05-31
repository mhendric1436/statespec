#include "common/memory/backend.hpp"
#include "common/runtime/workflow_store.hpp"
#include "worker/workflow_runner.hpp"
#include "worker/workflows/provision_service/handlers.hpp"
#include "worker/workflows/provision_service/registry.hpp"

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

class CountingWorkflowStore final : public statespec::backend::runtime::RuntimeWorkflowStore
{
  public:
    statespec::backend::WorkflowExecutionRecord keep_alive_step(
        statespec::backend::IBackend& backend,
        const statespec::backend::KeepAliveWorkflowStepRequest& request
    ) override
    {
        keep_alive_count_.fetch_add(1, std::memory_order_relaxed);
        return RuntimeWorkflowStore::keep_alive_step(backend, request);
    }

    int keep_alive_count() const
    {
        return keep_alive_count_.load(std::memory_order_relaxed);
    }

  private:
    std::atomic<int> keep_alive_count_{0};
};

class LongRunningWorkflowStepHandler final : public statespec_generated::worker::workflows::
                                                 provision_service::ProvisionServiceV1StepHandler
{
  public:
    statespec_generated::worker::WorkflowStepResult handle_validate_request(
        statespec::backend::ITransaction& tx,
        const statespec_generated::worker::WorkflowStepHandlerContext& context
    ) override
    {
        if (!tx.is_open())
        {
            throw std::runtime_error("workflow handler received a closed transaction");
        }
        if (context.workflow_name != "ProvisionService" || context.step_name != "validate_request")
        {
            throw std::runtime_error("unexpected workflow step");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{850});
        handled_ = true;
        return statespec_generated::worker::WorkflowStepResult::complete();
    }

    statespec_generated::worker::WorkflowStepResult handle_create_remote_service(
        statespec::backend::ITransaction& tx,
        const statespec_generated::worker::WorkflowStepHandlerContext&
    ) override
    {
        if (!tx.is_open())
        {
            throw std::runtime_error("workflow handler received a closed transaction");
        }
        return statespec_generated::worker::WorkflowStepResult::fail(
            "unexpected create_remote_service step"
        );
    }

    statespec_generated::worker::WorkflowStepResult handle_wait_for_remote_service(
        statespec::backend::ITransaction& tx,
        const statespec_generated::worker::WorkflowStepHandlerContext&
    ) override
    {
        if (!tx.is_open())
        {
            throw std::runtime_error("workflow handler received a closed transaction");
        }
        return statespec_generated::worker::WorkflowStepResult::fail(
            "unexpected wait_for_remote_service step"
        );
    }

    bool handled() const
    {
        return handled_;
    }

  private:
    bool handled_ = false;
};

int main()
{
    statespec::backend::memory::InMemoryBackend backend;
    CountingWorkflowStore workflows;

    workflows.register_definition(
        backend, statespec::backend::RegisterWorkflowDefinitionRequest{
                     statespec::backend::WorkflowDefinition{
                         "ProvisionService",
                         1,
                         "validate_request",
                         std::chrono::seconds{60},
                         false,
                         {
                             statespec::backend::WorkflowStepDefinition{
                                 "validate_request", std::chrono::seconds{1}, 3
                             },
                             statespec::backend::WorkflowStepDefinition{
                                 "create_remote_service", std::chrono::seconds{1}, 3
                             },
                             statespec::backend::WorkflowStepDefinition{
                                 "wait_for_remote_service", std::chrono::seconds{1}, 3
                             },
                         },
                         statespec::backend::Json::object({}),
                     },
                 }
    );
    workflows.start(
        backend, statespec::backend::StartWorkflowRequest{
                     "wf-long",
                     "ProvisionService",
                     1,
                     "validate_request",
                     statespec::backend::Json::object({}),
                 }
    );

    LongRunningWorkflowStepHandler handler;
    statespec_generated::worker::WorkflowStepInvokerMap invokers;
    statespec_generated::worker::workflows::provision_service::register_workflow_step_invokers(
        invokers, handler
    );
    statespec_generated::worker::WorkflowRunner runner{
        backend, workflows, invokers, "ProvisionWorker", std::chrono::seconds{1}, 3,
    };
    const auto completed = runner.run_once("wf-long", "ProvisionService", 1);
    if (!handler.handled() || !completed.has_value() || completed->status != "Completed")
    {
        throw std::runtime_error("long-running workflow step did not complete");
    }
    if (workflows.keep_alive_count() < 2)
    {
        throw std::runtime_error("long-running workflow step did not receive periodic keepalive");
    }
}
