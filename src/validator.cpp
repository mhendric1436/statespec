#include "statespec/validator.hpp"

#include "statespec/symbol_table.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace statespec
{

namespace
{

bool contains(
    const std::unordered_set<std::string>& values,
    const std::string& value
)
{
    return values.find(value) != values.end();
}

bool is_builtin_type(const std::string& type)
{
    static const std::unordered_set<std::string> builtin_types{
        "string", "bool",    "int",  "int32",     "int64",    "long",
        "double", "decimal", "json", "timestamp", "duration", "uuid",
    };

    if (!type.empty() && type.back() == '?')
    {
        return builtin_types.find(type.substr(0, type.size() - 1)) != builtin_types.end();
    }

    return builtin_types.find(type) != builtin_types.end();
}

bool is_positive_integer(int value)
{
    return value > 0;
}

bool is_non_negative_integer(int value)
{
    return value >= 0;
}

bool is_duration_literal(const std::string& value)
{
    if (value.size() < 3 || value.front() != 'P')
    {
        return false;
    }

    bool has_digit = false;
    bool has_unit = false;
    bool in_time = false;

    for (std::size_t i = 1; i < value.size(); ++i)
    {
        const char ch = value[i];
        if (std::isdigit(static_cast<unsigned char>(ch)) != 0)
        {
            has_digit = true;
            continue;
        }
        if (ch == 'T')
        {
            if (in_time)
            {
                return false;
            }
            in_time = true;
            continue;
        }
        if (ch == 'D' || ch == 'H' || ch == 'M' || ch == 'S')
        {
            has_unit = true;
            continue;
        }
        return false;
    }

    return has_digit && has_unit;
}

std::string queue_message_name(
    const QueueDecl& queue,
    const MessageDecl& message
)
{
    return queue.name + "." + message.name;
}

std::string workflow_step_name(
    const WorkflowDecl& workflow,
    const WorkflowStepDecl& step
)
{
    return workflow.name + "." + step.name;
}

std::unordered_set<std::string> field_names(const std::vector<FieldDecl>& fields)
{
    std::unordered_set<std::string> names;
    for (const auto& field : fields)
    {
        names.insert(field.name);
    }
    return names;
}

std::unordered_set<std::string> step_names(const WorkflowDecl& workflow)
{
    std::unordered_set<std::string> names;
    for (const auto& step : workflow.steps)
    {
        names.insert(step.name);
    }
    return names;
}

void duplicate_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& name
)
{
    diagnostics.error(range, "SSPEC3001", "duplicate declaration '" + name + "'");
}

void unknown_reference_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& kind,
    const std::string& name
)
{
    diagnostics.error(range, "SSPEC3002", "unknown " + kind + " reference '" + name + "'");
}

void required_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4001", subject + " must declare " + field);
}

void positive_integer_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4002", subject + " " + field + " must be a positive integer");
}

void non_negative_integer_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4003", subject + " " + field + " must be non-negative");
}

void duration_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4004", subject + " " + field + " must be an ISO-8601 duration");
}

void add_symbol(
    SymbolTable& symbols,
    DiagnosticBag& diagnostics,
    SymbolKind kind,
    const std::string& name,
    const SourceRange& range
)
{
    if (!symbols.insert(Symbol{kind, name, range}))
    {
        duplicate_error(diagnostics, range, name);
    }
}

void validate_field_duplicates(
    const std::vector<FieldDecl>& fields,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> names;
    for (const auto& field : fields)
    {
        if (!names.insert(field.name).second)
        {
            duplicate_error(diagnostics, field.range, field.name);
        }
    }
}

void validate_field_types(
    const std::vector<FieldDecl>& fields,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& field : fields)
    {
        if (!is_builtin_type(field.type) && !symbols.find(field.type).has_value())
        {
            unknown_reference_error(diagnostics, field.range, "type", field.type);
        }
    }
}

void validate_state_machine(
    const EntityDecl& entity,
    DiagnosticBag& diagnostics
)
{
    if (!entity.state_machine.has_value())
    {
        return;
    }

    const auto& state_machine = *entity.state_machine;
    std::unordered_set<std::string> states;
    for (const auto& state : state_machine.states)
    {
        if (!states.insert(state.name).second)
        {
            duplicate_error(diagnostics, state.range, state.name);
        }
    }

    if (state_machine.states.empty())
    {
        required_error(
            diagnostics, state_machine.range, "state_machine for entity '" + entity.name + "'",
            "at least one state"
        );
    }

    if (!state_machine.initial_state.has_value())
    {
        required_error(
            diagnostics, state_machine.range, "state_machine for entity '" + entity.name + "'",
            "an initial state"
        );
    }
    else if (!contains(states, *state_machine.initial_state))
    {
        unknown_reference_error(
            diagnostics, state_machine.range, "initial state", *state_machine.initial_state
        );
    }

    for (const auto& terminal : state_machine.terminal_states)
    {
        if (!contains(states, terminal))
        {
            unknown_reference_error(diagnostics, state_machine.range, "terminal state", terminal);
        }
    }

    for (const auto& transition : state_machine.transitions)
    {
        if (!contains(states, transition.from))
        {
            unknown_reference_error(
                diagnostics, transition.range, "transition source state", transition.from
            );
        }
        if (!contains(states, transition.to))
        {
            unknown_reference_error(
                diagnostics, transition.range, "transition target state", transition.to
            );
        }
    }
}

void validate_indexes(
    const EntityDecl& entity,
    const std::unordered_set<std::string>& fields,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> index_names;
    for (const auto& index : entity.indexes)
    {
        if (!index_names.insert(index.name).second)
        {
            duplicate_error(diagnostics, index.range, index.name);
        }

        if (index.fields.empty())
        {
            required_error(
                diagnostics, index.range, "index '" + index.name + "'", "at least one field"
            );
        }

        std::unordered_set<std::string> index_fields;
        for (const auto& field : index.fields)
        {
            if (!contains(fields, field))
            {
                unknown_reference_error(diagnostics, index.range, "index field", field);
            }
            if (!index_fields.insert(field).second)
            {
                duplicate_error(diagnostics, index.range, field);
            }
        }
    }
}

void validate_entities(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& entity : system.entities)
    {
        if (entity.key_fields.empty())
        {
            diagnostics.error(
                entity.range, "SSPEC3101", "entity '" + entity.name + "' must declare a key"
            );
        }
        if (entity.fields.empty())
        {
            required_error(diagnostics, entity.range, "entity '" + entity.name + "'", "fields");
        }

        validate_field_duplicates(entity.fields, diagnostics);
        validate_field_types(entity.fields, symbols, diagnostics);

        const auto fields = field_names(entity.fields);
        for (const auto& key_field : entity.key_fields)
        {
            if (!contains(fields, key_field))
            {
                unknown_reference_error(diagnostics, entity.range, "entity key field", key_field);
            }
        }

        validate_indexes(entity, fields, diagnostics);
        validate_state_machine(entity, diagnostics);
    }
}

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

void validate_workflows(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    for (const auto& workflow : system.workflows)
    {
        if (!workflow.version.has_value())
        {
            required_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'", "version"
            );
        }
        else if (!is_positive_integer(*workflow.version))
        {
            positive_integer_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'", "version"
            );
        }
        if (!workflow.start_step.has_value())
        {
            required_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'", "start step"
            );
        }
        if (workflow.steps.empty())
        {
            required_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'", "at least one step"
            );
        }
        if (workflow.expected_execution_time.has_value() &&
            !is_duration_literal(*workflow.expected_execution_time))
        {
            duration_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'",
                "expected_execution_time"
            );
        }

        std::unordered_set<std::string> steps;
        for (const auto& step : workflow.steps)
        {
            if (!steps.insert(step.name).second)
            {
                duplicate_error(diagnostics, step.range, workflow_step_name(workflow, step));
            }
            if (!step.expected_execution_time.has_value())
            {
                required_error(
                    diagnostics, step.range,
                    "workflow step '" + workflow_step_name(workflow, step) + "'",
                    "expected_execution_time"
                );
            }
            else if (!is_duration_literal(*step.expected_execution_time))
            {
                duration_error(
                    diagnostics, step.range,
                    "workflow step '" + workflow_step_name(workflow, step) + "'",
                    "expected_execution_time"
                );
            }
            if (step.max_retries.has_value() && !is_non_negative_integer(*step.max_retries))
            {
                non_negative_integer_error(
                    diagnostics, step.range,
                    "workflow step '" + workflow_step_name(workflow, step) + "'", "max_retries"
                );
            }
        }

        if (workflow.start_step.has_value() && !contains(steps, *workflow.start_step))
        {
            unknown_reference_error(
                diagnostics, workflow.range, "workflow start step", *workflow.start_step
            );
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

void validate_policies(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& policy : system.policies)
    {
        for (const auto& rule : policy.allows)
        {
            if (!symbols.find(rule.action).has_value())
            {
                unknown_reference_error(
                    diagnostics, rule.range, "policy allow action", rule.action
                );
            }
        }
        for (const auto& rule : policy.denies)
        {
            if (!symbols.find(rule.action).has_value())
            {
                unknown_reference_error(diagnostics, rule.range, "policy deny action", rule.action);
            }
        }
        for (const auto& audit : policy.audits)
        {
            if (!symbols.find(audit).has_value())
            {
                unknown_reference_error(diagnostics, policy.range, "policy audit action", audit);
            }
        }
    }
}

SymbolTable build_symbol_table(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    SymbolTable symbols;
    add_symbol(symbols, diagnostics, SymbolKind::System, system.name, system.range);

    for (const auto& entity : system.entities)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Entity, entity.name, entity.range);
    }
    for (const auto& queue : system.queues)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Queue, queue.name, queue.range);
        for (const auto& message : queue.messages)
        {
            add_symbol(
                symbols, diagnostics, SymbolKind::Message, queue_message_name(queue, message),
                message.range
            );
        }
    }
    for (const auto& lease : system.leases)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Lease, lease.name, lease.range);
    }
    for (const auto& worker : system.workers)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Worker, worker.name, worker.range);
    }
    for (const auto& api : system.apis)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Api, api.name, api.range);
    }
    for (const auto& workflow : system.workflows)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Workflow, workflow.name, workflow.range);
        for (const auto& step : workflow.steps)
        {
            add_symbol(
                symbols, diagnostics, SymbolKind::WorkflowStep, workflow_step_name(workflow, step),
                step.range
            );
        }
    }
    for (const auto& policy : system.policies)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Policy, policy.name, policy.range);
    }

    return symbols;
}

} // namespace

void Validator::validate(
    const Spec& spec,
    DiagnosticBag& diagnostics
)
{
    if (!spec.system.has_value())
    {
        diagnostics.error(SourceRange{}, "SSPEC1001", "spec must contain a system declaration");
        return;
    }

    const auto& system = *spec.system;
    auto symbols = build_symbol_table(system, diagnostics);

    validate_entities(system, symbols, diagnostics);
    validate_queues(system, symbols, diagnostics);
    validate_leases(system, diagnostics);
    validate_workflows(system, diagnostics);
    validate_workers(system, symbols, diagnostics);
    validate_apis(system, symbols, diagnostics);
    validate_policies(system, symbols, diagnostics);
}

} // namespace statespec
