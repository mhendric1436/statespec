#include "semantic_runtime.hpp"

#include "semantic_symbols.hpp"

#include <utility>

namespace statespec
{

void resolve_semantic_runtime(
    const SystemDecl& system,
    const SymbolTable& symbols,
    SemanticSystem& resolved
)
{
    for (const auto& queue : system.queues)
    {
        SemanticQueue resolved_queue;
        resolved_queue.name = queue.name;
        resolved_queue.namespace_name = queue.namespace_name;
        resolved_queue.channel = queue.channel;
        resolved_queue.visibility_timeout = queue.visibility_timeout;
        resolved_queue.max_attempts = queue.max_attempts;
        resolved_queue.dead_letter = resolve_optional_reference(symbols, queue.dead_letter);
        for (const auto& message : queue.messages)
        {
            resolved_queue.messages.push_back(
                SemanticMessage{
                    message.name,
                    message.idempotency_key,
                    resolve_fields(message.payload_fields),
                }
            );
        }
        resolved.queues.push_back(std::move(resolved_queue));
    }

    for (const auto& lease : system.leases)
    {
        resolved.leases.push_back(
            SemanticLease{
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
        resolved.workers.push_back(
            SemanticWorker{
                worker.name,
                worker.singleton,
                resolve_optional_reference(symbols, worker.lease),
                resolve_optional_reference(symbols, worker.polls),
                resolve_optional_reference(symbols, worker.executes),
                worker.concurrency,
            }
        );
    }

    for (const auto& api_server : system.api_servers)
    {
        SemanticApiServer resolved_api_server;
        resolved_api_server.name = api_server.name;
        resolved_api_server.concurrency = api_server.concurrency;
        for (const auto& served_api : api_server.serves)
        {
            resolved_api_server.serves.push_back(resolve_reference(symbols, served_api));
        }
        resolved.api_servers.push_back(std::move(resolved_api_server));
    }

    for (const auto& api : system.apis)
    {
        resolved.apis.push_back(
            SemanticApi{
                api.name,
                api.method,
                api.path,
                resolve_optional_reference(symbols, api.input),
                resolve_optional_reference(symbols, api.output),
                resolve_optional_reference(symbols, api.error),
                resolve_optional_reference(symbols, api.starts_workflow),
                resolve_optional_reference(symbols, api.enqueues),
            }
        );
    }
}

} // namespace statespec
