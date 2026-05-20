#include "common/memory/backend.hpp"
#include "common/memory/workflow_store.hpp"
#include "worker/workflow_runner.hpp"

#include <chrono>
#include <stdexcept>

class LinkingWorkflowStepHandler final : public statespec_generated::worker::IWorkflowStepHandler
{
  public:
    void handle(const statespec_generated::worker::WorkflowStepHandlerContext& context) override
    {
        if (context.workflow_name != "ProvisionService" ||
            context.step_name != "validate_request")
        {
            throw std::runtime_error("unexpected workflow step");
        }
        handled_ = true;
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
    statespec::backend::memory::InMemoryWorkflowStore workflows;

    workflows.register_definition(
        backend,
        statespec::backend::RegisterWorkflowDefinitionRequest{
            statespec::backend::WorkflowDefinition{
                "ProvisionService",
                1,
                "validate_request",
                std::chrono::seconds{60},
                false,
                {statespec::backend::WorkflowStepDefinition{
                    "validate_request",
                    std::chrono::seconds{1},
                    3,
                }},
                statespec::backend::Json::object({}),
            },
        }
    );
    workflows.start(
        backend,
        statespec::backend::StartWorkflowRequest{
            "wf-1",
            "ProvisionService",
            1,
            "validate_request",
            statespec::backend::Json::object({}),
        }
    );

    LinkingWorkflowStepHandler handler;
    statespec_generated::worker::WorkflowRunner runner{
        backend,
        workflows,
        handler,
        "ProvisionWorker",
        std::chrono::seconds{60},
        3,
    };
    const auto completed = runner.run_once("wf-1", "ProvisionService", 1);
    if (!handler.handled() || !completed.has_value() || completed->status != "Completed")
    {
        throw std::runtime_error("generated worker runner did not complete linked workflow step");
    }
}
