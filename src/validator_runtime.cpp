#include "validator_runtime.hpp"

#include "validator_declarations.hpp"
#include "validator_helpers.hpp"

#include <string>
#include <unordered_set>

namespace statespec::validator_detail
{

void validate_queues(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& queue : system.queues)
    {
        if (!queue.namespace_name.has_value())
        {
            required_error(diagnostics, queue.range, "queue '" + queue.name + "'", "namespace");
        }
        if (!queue.channel.has_value())
        {
            required_error(diagnostics, queue.range, "queue '" + queue.name + "'", "channel");
        }
        if (queue.visibility_timeout.has_value() && !is_duration_literal(*queue.visibility_timeout))
        {
            duration_error(
                diagnostics, queue.range, "queue '" + queue.name + "'", "visibility_timeout"
            );
        }
        if (queue.max_attempts.has_value() && !is_positive_integer(*queue.max_attempts))
        {
            positive_integer_error(
                diagnostics, queue.range, "queue '" + queue.name + "'", "max_attempts"
            );
        }
        if (queue.messages.empty())
        {
            required_error(
                diagnostics, queue.range, "queue '" + queue.name + "'", "at least one message"
            );
        }

        std::unordered_set<std::string> message_names;
        for (const auto& message : queue.messages)
        {
            if (!message_names.insert(message.name).second)
            {
                duplicate_error(diagnostics, message.range, queue_message_name(queue, message));
            }
            if (!message.idempotency_key.has_value())
            {
                required_error(
                    diagnostics, message.range,
                    "message '" + queue_message_name(queue, message) + "'", "idempotency_key"
                );
            }
            if (message.payload_fields.empty())
            {
                required_error(
                    diagnostics, message.range,
                    "message '" + queue_message_name(queue, message) + "'", "payload"
                );
            }

            validate_field_duplicates(message.payload_fields, diagnostics);
            validate_field_types(message.payload_fields, symbols, diagnostics);

            if (message.idempotency_key.has_value())
            {
                const auto payload_fields = field_names(message.payload_fields);
                if (!contains(payload_fields, *message.idempotency_key))
                {
                    unknown_reference_error(
                        diagnostics, message.range, "message idempotency_key field",
                        *message.idempotency_key
                    );
                }
            }
        }

        if (queue.dead_letter.has_value() && !symbols.find(*queue.dead_letter).has_value())
        {
            unknown_reference_error(diagnostics, queue.range, "dead_letter", *queue.dead_letter);
        }
    }
}

void validate_leases(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    for (const auto& lease : system.leases)
    {
        if (!lease.resource.has_value())
        {
            required_error(diagnostics, lease.range, "lease '" + lease.name + "'", "resource");
        }
        if (!lease.ttl.has_value())
        {
            required_error(diagnostics, lease.range, "lease '" + lease.name + "'", "ttl");
        }
        else if (!is_duration_literal(*lease.ttl))
        {
            duration_error(diagnostics, lease.range, "lease '" + lease.name + "'", "ttl");
        }
        if (lease.renew_every.has_value() && !is_duration_literal(*lease.renew_every))
        {
            duration_error(diagnostics, lease.range, "lease '" + lease.name + "'", "renew_every");
        }
        if (lease.max_ttl.has_value() && !is_duration_literal(*lease.max_ttl))
        {
            duration_error(diagnostics, lease.range, "lease '" + lease.name + "'", "max_ttl");
        }
    }
}

void validate_workers(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& worker : system.workers)
    {
        if (worker.singleton.value_or(false) && !worker.lease.has_value())
        {
            diagnostics.error(
                worker.range, "SSPEC3301",
                "singleton worker '" + worker.name + "' must declare a lease"
            );
        }
        if (worker.concurrency.has_value() && !is_positive_integer(*worker.concurrency))
        {
            positive_integer_error(
                diagnostics, worker.range, "worker '" + worker.name + "'", "concurrency"
            );
        }

        if (worker.lease.has_value() && !symbols.find(*worker.lease).has_value())
        {
            unknown_reference_error(diagnostics, worker.range, "worker lease", *worker.lease);
        }
        if (worker.polls.has_value() && !symbols.find(*worker.polls).has_value())
        {
            unknown_reference_error(
                diagnostics, worker.range, "worker polls target", *worker.polls
            );
        }
        if (worker.executes.has_value() && !symbols.find(*worker.executes).has_value())
        {
            unknown_reference_error(
                diagnostics, worker.range, "worker executes target", *worker.executes
            );
        }
    }
}

void validate_apis(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& api : system.apis)
    {
        if (!api.method.has_value())
        {
            required_error(diagnostics, api.range, "api '" + api.name + "'", "method");
        }
        if (!api.path.has_value())
        {
            required_error(diagnostics, api.range, "api '" + api.name + "'", "path");
        }
        if (api.input.has_value() && !is_known_type_reference(*api.input, symbols))
        {
            unknown_reference_error(diagnostics, api.range, "API input", *api.input);
        }
        if (api.output.has_value() && !is_known_type_reference(*api.output, symbols))
        {
            unknown_reference_error(diagnostics, api.range, "API output", *api.output);
        }
        if (api.error.has_value() && !is_known_type_reference(*api.error, symbols))
        {
            unknown_reference_error(diagnostics, api.range, "API error", *api.error);
        }
        if (api.starts_workflow.has_value() && !symbols.find(*api.starts_workflow).has_value())
        {
            unknown_reference_error(
                diagnostics, api.range, "API starts workflow", *api.starts_workflow
            );
        }
        if (api.enqueues.has_value() && !symbols.find(*api.enqueues).has_value())
        {
            unknown_reference_error(diagnostics, api.range, "API enqueues target", *api.enqueues);
        }
    }
}

void validate_api_servers(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& api_server : system.api_servers)
    {
        if (api_server.concurrency.has_value() && !is_positive_integer(*api_server.concurrency))
        {
            positive_integer_error(
                diagnostics, api_server.range, "api_server '" + api_server.name + "'", "concurrency"
            );
        }

        for (const auto& served_api : api_server.serves)
        {
            validate_symbol_reference(
                symbols, api_server.range, "api_server served API", served_api, {SymbolKind::Api},
                diagnostics
            );
        }
    }
}

} // namespace statespec::validator_detail
