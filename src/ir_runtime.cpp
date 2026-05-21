#include "ir_runtime.hpp"

#include "ir_lowering_helpers.hpp"

#include <utility>

namespace statespec
{

void lower_ir_runtime(
    const SemanticSystem& system,
    IrSystem& ir
)
{
    for (const auto& queue : system.queues)
    {
        IrQueue ir_queue;
        ir_queue.name = queue.name;
        ir_queue.namespace_name = queue.namespace_name;
        ir_queue.channel = queue.channel;
        ir_queue.visibility_timeout = queue.visibility_timeout;
        ir_queue.max_attempts = queue.max_attempts;
        ir_queue.dead_letter = reference_name(queue.dead_letter);
        for (const auto& message : queue.messages)
        {
            IrMessage ir_message;
            ir_message.name = message.name;
            ir_message.idempotency_key = message.idempotency_key;
            ir_message.payload_fields = lower_fields(message.payload_fields);
            ir_queue.messages.push_back(std::move(ir_message));
        }
        ir.queues.push_back(std::move(ir_queue));
    }

    for (const auto& lease : system.leases)
    {
        ir.leases.push_back(
            IrLease{
                lease.name,
                lease.resource,
                lease.ttl,
                lease.renew_every,
                lease.holder,
                lease.fencing_token,
                lease.max_ttl,
            }
        );
    }

    for (const auto& worker : system.workers)
    {
        ir.workers.push_back(
            IrWorker{
                worker.name,
                worker.singleton,
                reference_name(worker.lease),
                reference_name(worker.polls),
                reference_name(worker.executes),
                worker.concurrency,
            }
        );
    }

    for (const auto& api_server : system.api_servers)
    {
        ir.api_servers.push_back(
            IrApiServer{
                api_server.name,
                reference_names(api_server.serves),
                api_server.concurrency,
            }
        );
    }

    for (const auto& api : system.apis)
    {
        ir.apis.push_back(
            IrApi{
                api.name,
                api.method,
                api.path,
                reference_name(api.input),
                reference_name(api.output),
                reference_name(api.error),
                reference_name(api.starts_workflow),
                reference_name(api.enqueues),
            }
        );
    }
}

} // namespace statespec
