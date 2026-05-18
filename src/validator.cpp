#include "statespec/validator.hpp"

#include "validator_context.hpp"
#include "validator_declarations.hpp"
#include "validator_entities.hpp"
#include "validator_helpers.hpp"

#include "statespec/symbol_table.hpp"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

namespace statespec
{

using namespace validator_detail;

namespace
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

void validate_workflows(
    const SystemDecl& system,
    const SymbolTable& symbols,
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
        if (workflow.on.has_value())
        {
            validate_symbol_reference(
                symbols, workflow.range, "workflow trigger", *workflow.on,
                {SymbolKind::Api, SymbolKind::Event, SymbolKind::Message}, diagnostics
            );
        }
        if (workflow.input.has_value() && !is_known_type_reference(*workflow.input, symbols))
        {
            unknown_reference_error(diagnostics, workflow.range, "workflow input", *workflow.input);
        }
        if (workflow.state.has_value() && !is_known_type_reference(*workflow.state, symbols))
        {
            unknown_reference_error(diagnostics, workflow.range, "workflow state", *workflow.state);
        }

        std::unordered_set<std::string> load_bindings;
        for (const auto& load : workflow.loads)
        {
            if (!load_bindings.insert(load.binding).second)
            {
                duplicate_error(diagnostics, load.range, load.binding);
            }

            const auto* entity = find_entity(system, load.entity);
            if (entity == nullptr)
            {
                unknown_reference_error(
                    diagnostics, load.range, "workflow load entity", load.entity
                );
                continue;
            }

            const auto key_fields = std::unordered_set<std::string>{
                entity->key_fields.begin(), entity->key_fields.end()
            };
            if (!contains(key_fields, load.key_field))
            {
                unknown_reference_error(
                    diagnostics, load.range, "workflow load key field", load.key_field
                );
            }
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

            for (const auto& statement : step.statements)
            {
                if (statement.expression.has_value())
                {
                    validate_feature_flag_expression(
                        system, statement.range, *statement.expression, diagnostics
                    );
                }
                for (const auto& assignment : statement.payload)
                {
                    validate_feature_flag_expression(
                        system, assignment.range, assignment.expression, diagnostics
                    );
                }

                if (statement.kind == "emit" && statement.target.has_value())
                {
                    validate_symbol_reference(
                        symbols, statement.range, "workflow emit target", *statement.target,
                        {SymbolKind::Event, SymbolKind::Log}, diagnostics
                    );
                }
                else if (statement.kind == "enqueue" && statement.target.has_value())
                {
                    validate_symbol_reference(
                        symbols, statement.range, "workflow enqueue target", *statement.target,
                        {SymbolKind::Message}, diagnostics
                    );
                }
                else if ((statement.kind == "acquire_lease" || statement.kind == "renew_lease" ||
                          statement.kind == "release_lease") &&
                         statement.target.has_value())
                {
                    validate_symbol_reference(
                        symbols, statement.range, "workflow lease target", *statement.target,
                        {SymbolKind::Lease}, diagnostics
                    );
                }
                else if (statement.kind == "start_workflow" && statement.target.has_value())
                {
                    validate_symbol_reference(
                        symbols, statement.range, "workflow start target", *statement.target,
                        {SymbolKind::Workflow}, diagnostics
                    );
                }
            }
        }

        if (workflow.start_step.has_value() && !contains(steps, *workflow.start_step))
        {
            unknown_reference_error(
                diagnostics, workflow.range, "workflow start step", *workflow.start_step
            );
        }
        for (const auto& step : workflow.steps)
        {
            for (const auto& statement : step.statements)
            {
                if (statement.kind == "transition_to" && statement.target.has_value() &&
                    !contains(steps, *statement.target))
                {
                    unknown_reference_error(
                        diagnostics, statement.range, "workflow transition target",
                        *statement.target
                    );
                }
            }
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
    const ValidatorContext context{spec, system, symbols, diagnostics};

    validate_feature_flags(context.system, context.symbols, context.diagnostics);
    validate_namespaces(context.system, context.symbols, context.diagnostics);
    validate_values(context.system, context.symbols, context.diagnostics);
    validate_enums(context.diagnostics, context.system);
    validate_events(context.system, context.symbols, context.diagnostics);
    validate_external_systems(context.system, context.diagnostics);
    validate_shapes(context.system, context.symbols, context.diagnostics);
    validate_logs(context.system, context.symbols, context.diagnostics);
    validate_metrics(context.system, context.symbols, context.diagnostics);
    validate_entities(context.system, context.symbols, context.diagnostics);
    validate_queues(context.system, context.symbols, context.diagnostics);
    validate_leases(context.system, context.diagnostics);
    validate_workflows(context.system, context.symbols, context.diagnostics);
    validate_workers(context.system, context.symbols, context.diagnostics);
    validate_apis(context.system, context.symbols, context.diagnostics);
    validate_api_servers(context.system, context.symbols, context.diagnostics);
    validate_policies(context.system, context.symbols, context.diagnostics);
}

} // namespace statespec
