#pragma once

#include "worker_handlers.hpp"
#include "worker_registry.hpp"

#include <utility>

namespace statespec_generated::worker
{

class WorkerApplication
{
  public:
    WorkerApplication(
        WorkerContext context,
        IWorker& handler
    )
        : context_(std::move(context)),
          handler_(handler)
    {
    }

    const WorkerContext& context() const
    {
        return context_;
    }

    void run()
    {
        handler_.run(context_);
    }

  private:
    WorkerContext context_;
    IWorker& handler_;
};

} // namespace statespec_generated::worker
