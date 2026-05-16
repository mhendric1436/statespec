#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"

namespace
{

void parser_parses_empty_system()
{
    const auto spec = statespec::test::parse_text("statespec 0.1; system OrderSystem {}");
    statespec::test::require(spec.version == "0.1", "parser should capture version");
    statespec::test::require(spec.system.has_value(), "parser should capture system");
    statespec::test::require(
        spec.system->name == "OrderSystem", "parser should capture system name"
    );
}

void parser_parses_include_declarations()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        statespec 0.1;
        include "./workflow-launch-control.sspec";
        include "../shared/tenant-model.sspec";

        system OrderSystem {}
    )sspec");

    statespec::test::require(spec.includes.size() == 2, "parser should parse includes");
    statespec::test::require(
        spec.includes[0].path == "./workflow-launch-control.sspec",
        "parser should strip quotes from include path"
    );
    statespec::test::require(
        spec.includes[1].path == "../shared/tenant-model.sspec",
        "parser should preserve relative include path"
    );
    statespec::test::require(spec.system.has_value(), "parser should still parse root system");
}

void parser_parses_entity_fields_and_state_machine()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          entity Order {
            key tenant_id, order_id
            fields {
              tenant_id string
              order_id string
              status string
            }
            indexes {
              index by_tenant_status on tenant_id, status
              unique by_tenant_order on tenant_id, order_id
            }
            state_machine {
              state Creating
              state Active
              state Failed {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Creating
              terminal [Failed]
              Creating -> Active
              Creating -> Failed
            }
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(spec.system->entities.size() == 1, "parser should parse one entity");
    const auto& entity = spec.system->entities[0];
    statespec::test::require(entity.name == "Order", "parser should parse entity name");
    statespec::test::require(
        entity.key_fields.size() == 2, "parser should parse composite key fields"
    );
    statespec::test::require(entity.fields.size() == 3, "parser should parse fields");
    statespec::test::require(
        entity.fields[0].name == "tenant_id", "parser should parse field name"
    );
    statespec::test::require(entity.fields[0].type == "string", "parser should parse field type");
    statespec::test::require(entity.indexes.size() == 2, "parser should parse indexes");
    statespec::test::require(
        entity.indexes[0].name == "by_tenant_status", "parser should parse index name"
    );
    statespec::test::require(
        entity.indexes[0].fields.size() == 2, "parser should parse index fields"
    );
    statespec::test::require(!entity.indexes[0].unique, "parser should parse non-unique index");
    statespec::test::require(entity.indexes[1].unique, "parser should parse unique index");
    statespec::test::require(entity.state_machine.has_value(), "parser should parse state machine");
    statespec::test::require(
        entity.state_machine->states.size() == 3, "parser should parse states"
    );
    statespec::test::require(
        entity.state_machine->states[2].terminal, "parser should parse terminal state option"
    );
    statespec::test::require(
        entity.state_machine->states[2].garbage_collection.has_value(),
        "parser should parse garbage collection policy"
    );
    statespec::test::require(
        entity.state_machine->states[2].garbage_collection->after == "P30D",
        "parser should parse garbage collection duration"
    );
    statespec::test::require(
        entity.state_machine->states[2].garbage_collection->mode == "tombstone",
        "parser should parse garbage collection mode"
    );
    statespec::test::require(
        entity.state_machine->initial_state == "Creating", "parser should parse initial state"
    );
    statespec::test::require(
        entity.state_machine->terminal_states.size() == 1, "parser should parse terminal states"
    );
    statespec::test::require(
        entity.state_machine->transitions.size() == 2, "parser should parse transitions"
    );
}

void parser_parses_entity_ownership_relations_children_and_invariants()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          entity Account {
            key account_id
            children {
              orders: Order by account_id
            }
            fields {
              account_id string
              created_at timestamp
              updated_at timestamp
              status string
            }
            state_machine {
              state Active
              initial Active
            }
          }

          entity Order {
            key order_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            relations {
              parent account_id: ref<Account> {
                kind: composition
                on_parent_delete: block
                parent_must_be_in: [Active]
                unique_within_parent: [order_id]
              }
            }
            fields {
              order_id string
              account_id string
              created_at timestamp
              updated_at timestamp
              status string
            }
            invariants {
              valid_status: status != ""
            }
            state_machine {
              state Pending
              state Active
              initial Pending
              Pending -> Active
            }
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(spec.system->entities.size() == 2, "parser should parse entities");

    const auto& account = spec.system->entities[0];
    statespec::test::require(account.children.size() == 1, "parser should parse children");
    statespec::test::require(
        account.children[0].name == "orders", "parser should parse child name"
    );
    statespec::test::require(
        account.children[0].target_entity == "Order", "parser should parse child target"
    );
    statespec::test::require(
        account.children[0].relation == "account_id", "parser should parse child relation"
    );

    const auto& order = spec.system->entities[1];
    statespec::test::require(order.ownership.has_value(), "parser should parse ownership");
    statespec::test::require(
        order.ownership->authority == "system", "parser should parse ownership authority"
    );
    statespec::test::require(
        order.ownership->system_of_record == "self",
        "parser should parse ownership system_of_record"
    );
    statespec::test::require(
        order.ownership->lifecycle == "authoritative", "parser should parse ownership lifecycle"
    );
    statespec::test::require(order.relations.size() == 1, "parser should parse relations");
    const auto& relation = order.relations[0];
    statespec::test::require(relation.kind == "parent", "parser should parse relation kind");
    statespec::test::require(relation.name == "account_id", "parser should parse relation name");
    statespec::test::require(
        relation.target == "ref<Account>", "parser should parse relation target"
    );
    statespec::test::require(
        relation.relation_kind == "composition", "parser should parse relation options"
    );
    statespec::test::require(
        relation.parent_must_be_in.size() == 1 && relation.parent_must_be_in[0] == "Active",
        "parser should parse parent state guard"
    );
    statespec::test::require(
        relation.unique_within_parent.size() == 1 && relation.unique_within_parent[0] == "order_id",
        "parser should parse unique_within_parent"
    );
    statespec::test::require(order.invariants.size() == 1, "parser should parse invariants");
    statespec::test::require(
        order.invariants[0].name == "valid_status", "parser should parse invariant name"
    );
    statespec::test::require(
        !order.invariants[0].expression.empty(), "parser should parse invariant expression"
    );
}

void parser_parses_feature_flags()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          feature_flag NewScheduler {
            type bool
            default false
            scope tenant
            owner "platform"
            description "Route order processing through the new scheduler"
            expires "2026-12-31"
          }

          feature_flag MaxPendingOrders {
            type int
            default 100
            scope entity Order
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(
        spec.system->feature_flags.size() == 2, "parser should parse feature flags"
    );

    const auto& bool_flag = spec.system->feature_flags[0];
    statespec::test::require(
        bool_flag.name == "NewScheduler", "parser should parse feature flag name"
    );
    statespec::test::require(bool_flag.type == "bool", "parser should parse feature flag type");
    statespec::test::require(
        bool_flag.default_value == "false", "parser should parse boolean default"
    );
    statespec::test::require(bool_flag.scope == "tenant", "parser should parse tenant scope");
    statespec::test::require(bool_flag.owner == "platform", "parser should parse owner");
    statespec::test::require(
        bool_flag.description == "Route order processing through the new scheduler",
        "parser should parse description"
    );
    statespec::test::require(bool_flag.expires == "2026-12-31", "parser should parse expiration");

    const auto& typed_flag = spec.system->feature_flags[1];
    statespec::test::require(
        typed_flag.name == "MaxPendingOrders", "parser should parse second flag name"
    );
    statespec::test::require(typed_flag.type == "int", "parser should parse typed flag type");
    statespec::test::require(
        typed_flag.default_value == "100", "parser should parse integer default"
    );
    statespec::test::require(
        typed_flag.scope == "entity Order", "parser should parse entity scope"
    );
}

void parser_parses_shapes()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          shape CreateOrderRequest {
            tenant_id string
            order_id string
            priority int?
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(spec.system->shapes.size() == 1, "parser should parse shapes");
    const auto& shape = spec.system->shapes[0];
    statespec::test::require(shape.name == "CreateOrderRequest", "parser should parse shape name");
    statespec::test::require(shape.fields.size() == 3, "parser should parse shape fields");
    statespec::test::require(
        shape.fields[0].name == "tenant_id", "parser should parse shape field name"
    );
    statespec::test::require(
        shape.fields[2].type == "int?", "parser should parse optional shape field type"
    );
}

void parser_parses_logs_and_metrics()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          log WorkflowLaunchDecision {
            level info
            event_name "workflow.launch.decision"
            fields {
              tenant_id string
              reservation_id string
              decision string
            }
          }

          metric WorkflowLaunchAttempts {
            kind counter
            name "workflow_launch_attempts_total"
            unit count
            labels {
              tenant_id string
              workflow_name string
              decision string
            }
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(spec.system->logs.size() == 1, "parser should parse one log");
    const auto& log = spec.system->logs[0];
    statespec::test::require(log.name == "WorkflowLaunchDecision", "parser should parse log name");
    statespec::test::require(log.level == "info", "parser should parse log level");
    statespec::test::require(
        log.event_name == "workflow.launch.decision", "parser should parse log event name"
    );
    statespec::test::require(log.fields.size() == 3, "parser should parse log fields");
    statespec::test::require(
        log.fields[0].name == "tenant_id", "parser should parse log field name"
    );
    statespec::test::require(log.fields[0].type == "string", "parser should parse log field type");

    statespec::test::require(spec.system->metrics.size() == 1, "parser should parse one metric");
    const auto& metric = spec.system->metrics[0];
    statespec::test::require(
        metric.name == "WorkflowLaunchAttempts", "parser should parse metric name"
    );
    statespec::test::require(metric.kind == "counter", "parser should parse metric kind");
    statespec::test::require(
        metric.backend_name == "workflow_launch_attempts_total",
        "parser should parse metric backend name"
    );
    statespec::test::require(metric.unit == "count", "parser should parse metric unit");
    statespec::test::require(metric.labels.size() == 3, "parser should parse metric labels");
    statespec::test::require(
        metric.labels[1].name == "workflow_name", "parser should parse metric label name"
    );
    statespec::test::require(
        metric.labels[1].type == "string", "parser should parse metric label type"
    );
}

void parser_parses_workflow_steps()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          workflow OrderProcessing {
            version 1
            singleton false
            expected_execution_time PT5M
            start validate_order
            step validate_order {
              expected_execution_time PT10S
              max_retries 2
            }
            step charge_payment {
              expected_execution_time PT30S
              max_retries 3
            }
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(
        spec.system->workflows.size() == 1, "parser should parse one workflow"
    );
    const auto& workflow = spec.system->workflows[0];
    statespec::test::require(
        workflow.name == "OrderProcessing", "parser should parse workflow name"
    );
    statespec::test::require(workflow.version == 1, "parser should parse workflow version");
    statespec::test::require(workflow.singleton == false, "parser should parse singleton value");
    statespec::test::require(
        workflow.expected_execution_time == "PT5M", "parser should parse workflow duration"
    );
    statespec::test::require(
        workflow.start_step == "validate_order", "parser should parse start step"
    );
    statespec::test::require(workflow.steps.size() == 2, "parser should parse workflow steps");
    statespec::test::require(
        workflow.steps[0].name == "validate_order", "parser should parse first step name"
    );
    statespec::test::require(
        workflow.steps[0].expected_execution_time == "PT10S", "parser should parse step duration"
    );
    statespec::test::require(workflow.steps[0].max_retries == 2, "parser should parse max_retries");
}

void parser_parses_queue_and_message()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          queue EmailQueue {
            namespace workflow_ns
            channel email
            visibility_timeout PT30S
            max_attempts 5
            dead_letter EmailQueue.DeadLetter
            message SendConfirmation {
              idempotency_key message_id
              payload {
                message_id string
                order_id string
                email string
              }
            }
          }
        }
    )sspec");

    statespec::test::require(spec.system->queues.size() == 1, "parser should parse one queue");
    const auto& queue = spec.system->queues[0];
    statespec::test::require(queue.name == "EmailQueue", "parser should parse queue name");
    statespec::test::require(
        queue.namespace_name == "workflow_ns", "parser should parse queue namespace"
    );
    statespec::test::require(queue.channel == "email", "parser should parse queue channel");
    statespec::test::require(
        queue.visibility_timeout == "PT30S", "parser should parse visibility timeout"
    );
    statespec::test::require(queue.max_attempts == 5, "parser should parse max_attempts");
    statespec::test::require(
        queue.dead_letter == "EmailQueue.DeadLetter", "parser should parse dead_letter"
    );
    statespec::test::require(queue.messages.size() == 1, "parser should parse queue message");
    statespec::test::require(
        queue.messages[0].name == "SendConfirmation", "parser should parse message name"
    );
    statespec::test::require(
        queue.messages[0].idempotency_key == "message_id", "parser should parse idempotency key"
    );
    statespec::test::require(
        queue.messages[0].payload_fields.size() == 3, "parser should parse payload fields"
    );
}

void parser_parses_lease_and_worker()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          lease OrderReconcilerLease {
            resource "reconciler:orders"
            ttl PT30S
            renew_every PT10S
            holder worker_id
            fencing_token true
            max_ttl PT5M
          }
          worker OrderReconciler {
            singleton true
            lease OrderReconcilerLease
            polls EmailQueue.SendConfirmation
            executes OrderProcessing
            concurrency 4
          }
        }
    )sspec");

    statespec::test::require(spec.system->leases.size() == 1, "parser should parse one lease");
    const auto& lease = spec.system->leases[0];
    statespec::test::require(
        lease.name == "OrderReconcilerLease", "parser should parse lease name"
    );
    statespec::test::require(
        lease.resource == "reconciler:orders", "parser should parse lease resource"
    );
    statespec::test::require(lease.ttl == "PT30S", "parser should parse lease ttl");
    statespec::test::require(lease.renew_every == "PT10S", "parser should parse renew_every");
    statespec::test::require(lease.holder == "worker_id", "parser should parse holder");
    statespec::test::require(lease.fencing_token == true, "parser should parse fencing_token");
    statespec::test::require(lease.max_ttl == "PT5M", "parser should parse max_ttl");

    statespec::test::require(spec.system->workers.size() == 1, "parser should parse one worker");
    const auto& worker = spec.system->workers[0];
    statespec::test::require(worker.name == "OrderReconciler", "parser should parse worker name");
    statespec::test::require(worker.singleton == true, "parser should parse worker singleton");
    statespec::test::require(
        worker.lease == "OrderReconcilerLease", "parser should parse worker lease"
    );
    statespec::test::require(
        worker.polls == "EmailQueue.SendConfirmation", "parser should parse worker polls"
    );
    statespec::test::require(
        worker.executes == "OrderProcessing", "parser should parse worker executes"
    );
    statespec::test::require(worker.concurrency == 4, "parser should parse worker concurrency");
}

void parser_parses_api_and_policy()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          api StartOrderProcessing {
            method POST
            path "/v1/orders/{orderId}/start"
            input StartOrderRequest
            output StartOrderResponse
            error ProblemDetails
            starts workflow OrderProcessing
            enqueues EmailQueue.SendConfirmation
          }
          policy WorkflowAccess {
            tenant scoped_by tenant_id
            allow StartOrderProcessing when caller.role == operator;
            deny StartOrderProcessing when caller.suspended == true;
            quota starts_per_minute: 60;
            audit StartOrderProcessing;
          }
        }
    )sspec");

    statespec::test::require(spec.system->apis.size() == 1, "parser should parse one API");
    const auto& api = spec.system->apis[0];
    statespec::test::require(api.name == "StartOrderProcessing", "parser should parse API name");
    statespec::test::require(api.method == "POST", "parser should parse API method");
    statespec::test::require(
        api.path == "/v1/orders/{orderId}/start", "parser should parse API path"
    );
    statespec::test::require(api.input == "StartOrderRequest", "parser should parse API input");
    statespec::test::require(api.output == "StartOrderResponse", "parser should parse API output");
    statespec::test::require(api.error == "ProblemDetails", "parser should parse API error");
    statespec::test::require(
        api.starts_workflow == "OrderProcessing", "parser should parse started workflow"
    );
    statespec::test::require(
        api.enqueues == "EmailQueue.SendConfirmation", "parser should parse enqueued message"
    );

    statespec::test::require(spec.system->policies.size() == 1, "parser should parse one policy");
    const auto& policy = spec.system->policies[0];
    statespec::test::require(policy.name == "WorkflowAccess", "parser should parse policy name");
    statespec::test::require(
        policy.tenant_scoped_by == "tenant_id", "parser should parse tenant scope"
    );
    statespec::test::require(policy.allows.size() == 1, "parser should parse allow rule");
    statespec::test::require(policy.denies.size() == 1, "parser should parse deny rule");
    statespec::test::require(policy.quotas.size() == 1, "parser should parse quota");
    statespec::test::require(policy.audits.size() == 1, "parser should parse audit");
}

void parser_reports_missing_system()
{
    statespec::DiagnosticBag diagnostics;
    auto spec = statespec::test::parse_text("entity Order {}", diagnostics);
    statespec::test::require(
        !spec.system.has_value(), "parser should not synthesize missing system"
    );
    statespec::test::require(diagnostics.has_errors(), "parser should report missing system");
}

} // namespace

TEST_CASE("parser parses empty systems")
{
    parser_parses_empty_system();
}

TEST_CASE("parser parses include declarations")
{
    parser_parses_include_declarations();
}

TEST_CASE("parser parses entity fields, indexes, and state machines")
{
    parser_parses_entity_fields_and_state_machine();
}

TEST_CASE("parser parses entity ownership, relations, children, and invariants")
{
    parser_parses_entity_ownership_relations_children_and_invariants();
}

TEST_CASE("parser parses feature flags")
{
    parser_parses_feature_flags();
}

TEST_CASE("parser parses shapes")
{
    parser_parses_shapes();
}

TEST_CASE("parser parses logs and metrics")
{
    parser_parses_logs_and_metrics();
}

TEST_CASE("parser parses workflow steps")
{
    parser_parses_workflow_steps();
}

TEST_CASE("parser parses queues and messages")
{
    parser_parses_queue_and_message();
}

TEST_CASE("parser parses leases and workers")
{
    parser_parses_lease_and_worker();
}

TEST_CASE("parser parses APIs and policies")
{
    parser_parses_api_and_policy();
}

TEST_CASE("parser reports missing systems")
{
    parser_reports_missing_system();
}
