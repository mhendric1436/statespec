#include "common/memory/backend.hpp"
#include "common/runtime/workflow_store.hpp"
#include "worker/workflow_runner.hpp"
#include "worker/workflows/provision_service/handlers.hpp"
#include "worker/workflows/provision_service/registry.hpp"

#include <chrono>
#include <stdexcept>

class LinkingWorkflowStepHandler final : public statespec_generated::worker::workflows::
                                             provision_service::ProvisionServiceV1StepHandler
{
  public:
    statespec_generated::worker::WorkflowStepResult handle_validate_request(
        statespec::backend::ITransaction&,
        const statespec_generated::worker::WorkflowStepHandlerContext& context
    ) override
    {
        if (context.workflow_name != "ProvisionService" || context.step_name != "validate_request")
        {
            throw std::runtime_error("unexpected workflow step");
        }
        handled_ = true;
        return statespec_generated::worker::WorkflowStepResult::complete("create_remote_service");
    }

    statespec_generated::worker::WorkflowStepResult handle_create_remote_service(
        statespec::backend::ITransaction&,
        const statespec_generated::worker::WorkflowStepHandlerContext&
    ) override
    {
        return statespec_generated::worker::WorkflowStepResult::fail(
            "unexpected create_remote_service step"
        );
    }

    statespec_generated::worker::WorkflowStepResult handle_wait_for_remote_service(
        statespec::backend::ITransaction&,
        const statespec_generated::worker::WorkflowStepHandlerContext&
    ) override
    {
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
    statespec::backend::runtime::RuntimeWorkflowStore workflows;

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
                     "wf-1",
                     "ProvisionService",
                     1,
                     "validate_request",
                     statespec::backend::Json::object({}),
                 }
    );

    LinkingWorkflowStepHandler handler;
    statespec_generated::worker::WorkflowStepInvokerMap invokers;
    statespec_generated::worker::workflows::provision_service::register_workflow_step_invokers(
        invokers, handler
    );
    statespec_generated::worker::WorkflowRunner runner{
        backend, workflows, invokers, "ProvisionWorker", std::chrono::seconds{60}, 3,
    };
    const auto advanced = runner.run_once("wf-1", "ProvisionService", 1);
    if (!handler.handled() || !advanced.has_value() || advanced->status != "Running" ||
        advanced->current_step != "create_remote_service")
    {
        throw std::runtime_error("generated worker runner did not advance linked workflow step");
    }
}
