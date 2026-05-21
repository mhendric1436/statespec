#include "validator_runtime.hpp"

#include "validator_declarations.hpp"
#include "validator_helpers.hpp"

#include <unordered_set>

namespace statespec::validator_detail
{

namespace
{

void validate_queue_message_tenant_field(
    const SystemDecl& system,
    const QueueDecl& queue,
    const MessageDecl& message,
    DiagnosticBag& diagnostics
)
{
    if (!system.tenant_scope.has_value())
    {
        return;
    }

    const auto payload_fields = field_names(message.payload_fields);
    const auto& tenant_field = system.tenant_scope->field_name;
    if (!contains(payload_fields, tenant_field))
    {
        diagnostics.error(
            message.range, "SSPEC3402",
            "message '" + queue_message_name(queue, message) +
                "' payload must declare tenant field '" + tenant_field + "'"
        );
    }
}
} // namespace

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
        if (!queue.visibility_timeout.has_value())
        {
            required_error(
                diagnostics, queue.range, "queue '" + queue.name + "'", "visibility_timeout"
            );
        }
        else if (!is_duration_literal(*queue.visibility_timeout))
        {
            duration_error(
                diagnostics, queue.range, "queue '" + queue.name + "'", "visibility_timeout"
            );
        }
        if (!queue.max_attempts.has_value())
        {
            required_error(diagnostics, queue.range, "queue '" + queue.name + "'", "max_attempts");
        }
        else if (!is_positive_integer(*queue.max_attempts))
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
            validate_queue_message_tenant_field(system, queue, message, diagnostics);

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
} // namespace statespec::validator_detail
