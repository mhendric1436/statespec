#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/semantic.hpp"

namespace
{

void semantic_resolver_resolves_runtime_references()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          shape StartOrderProcessingRequest {
            order_id string
          }

          shape StartOrderProcessingResponse {
            accepted bool
          }

          value OrderAmount: decimal where OrderAmount >= 0;

          enum OrderStatus {
            Pending = "pending"
            Complete = "complete"
          }

          namespace Billing {
            external_system Stripe {
              owner: "payments"
            }

            shape InvoiceRequest {
              invoice_id uuid
            }
          }

          event OrderAccepted {
            fields {
              order_id string
              amount OrderAmount
              status OrderStatus
            }
          }

          entity Order {
            key order_id
            fields {
              order_id string
              created_at timestamp
              updated_at timestamp
              status string
            }
          }

          feature_flag OrderScopedFlag {
            type bool
            default false
            scope entity Order
          }

          queue EmailDispatch {
            namespace workflow_ns
            channel email
            visibility_timeout PT30S
            max_attempts 3
            message SendConfirmation {
              idempotency_key message_id
              payload {
                message_id string
                order_id string
              }
            }
          }

          lease OrderWorkerLease {
            resource "orders:worker"
            ttl PT30S
            renew_every PT10S
            holder worker_id
            fencing_token true
            max_ttl PT5M
          }

          worker OrderWorker {
            singleton true
            lease OrderWorkerLease
            polls EmailDispatch.SendConfirmation
            executes OrderProcessing
            concurrency 2
          }

          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/{order_id}/start"
            input StartOrderProcessingRequest
            output StartOrderProcessingResponse
            starts workflow OrderProcessing
          }

          api_server OrderApi {
            serves StartOrderProcessing
            concurrency 16
          }

          workflow OrderProcessing {
            version 1
            on OrderAccepted
            singleton false
            expected_execution_time PT60S
            start validate_order
            step validate_order {
              expected_execution_time PT10S
              max_retries 1
            }
          }

          policy OrderAccess {
            tenant scoped_by tenant_id
            allow StartOrderProcessing when caller.role == operator;
            audit StartOrderProcessing;
          }
        }
    )sspec");

    const auto resolved = statespec::resolve_semantics(spec);

    statespec::test::require(
        resolved.feature_flags[0].scope_target.has_value(),
        "semantic resolver should resolve entity-scoped feature flag target"
    );
    statespec::test::require(
        resolved.feature_flags[0].scope_target->kind == statespec::SymbolKind::Entity,
        "semantic resolver should annotate feature flag entity target kind"
    );

    const auto& worker = resolved.workers[0];
    statespec::test::require(
        worker.lease.has_value() && worker.lease->kind == statespec::SymbolKind::Lease,
        "semantic resolver should resolve worker lease"
    );
    statespec::test::require(
        worker.polls.has_value() && worker.polls->kind == statespec::SymbolKind::Message,
        "semantic resolver should resolve worker message target"
    );
    statespec::test::require(
        worker.executes.has_value() && worker.executes->kind == statespec::SymbolKind::Workflow,
        "semantic resolver should resolve worker workflow target"
    );

    const auto& api_server = resolved.api_servers[0];
    statespec::test::require(
        api_server.serves.size() == 1, "semantic resolver should lower API server serves"
    );
    statespec::test::require(
        api_server.serves[0].kind == statespec::SymbolKind::Api,
        "semantic resolver should resolve API server served API"
    );
    statespec::test::require(
        api_server.concurrency == 16, "semantic resolver should preserve API server concurrency"
    );

    const auto& api = resolved.apis[0];
    statespec::test::require(resolved.values.size() == 1, "semantic resolver should lower values");
    statespec::test::require(resolved.enums.size() == 1, "semantic resolver should lower enums");
    statespec::test::require(resolved.events.size() == 1, "semantic resolver should lower events");
    statespec::test::require(
        resolved.namespaces.size() == 1, "semantic resolver should lower namespaces"
    );
    statespec::test::require(
        resolved.namespaces[0].members[0].kind == statespec::SymbolKind::ExternalSystem,
        "semantic resolver should resolve namespace external system members"
    );
    statespec::test::require(
        resolved.external_systems.size() == 1, "semantic resolver should lower external systems"
    );
    statespec::test::require(
        resolved.events[0].fields[1].type == "OrderAmount",
        "semantic resolver should preserve event value field type"
    );
    statespec::test::require(resolved.shapes.size() == 3, "semantic resolver should lower shapes");
    statespec::test::require(
        api.input.has_value() && api.input->kind == statespec::SymbolKind::Shape,
        "semantic resolver should resolve API input shape"
    );
    statespec::test::require(
        api.output.has_value() && api.output->kind == statespec::SymbolKind::Shape,
        "semantic resolver should resolve API output shape"
    );
    statespec::test::require(
        api.starts_workflow.has_value() &&
            api.starts_workflow->kind == statespec::SymbolKind::Workflow,
        "semantic resolver should resolve API workflow target"
    );
    statespec::test::require(
        !api.enqueues.has_value(), "semantic resolver should omit API queue target"
    );

    const auto& workflow = resolved.workflows[0];
    statespec::test::require(
        workflow.on.has_value() && workflow.on->kind == statespec::SymbolKind::Event,
        "semantic resolver should resolve workflow event trigger"
    );

    const auto& policy = resolved.policies[0];
    statespec::test::require(
        policy.allows[0].action.kind == statespec::SymbolKind::Api,
        "semantic resolver should resolve policy allow action"
    );
    statespec::test::require(
        policy.audits[0].kind == statespec::SymbolKind::Api,
        "semantic resolver should resolve policy audit action"
    );
}

void semantic_resolver_preserves_unresolved_references()
{
    statespec::DiagnosticBag diagnostics;
    const auto spec = statespec::test::parse_text(
        R"sspec(
        system OrderSystem {
          worker BrokenWorker {
            polls MissingQueue.MissingMessage
          }
        }
    )sspec",
        diagnostics
    );

    const auto resolved = statespec::resolve_semantics(spec);

    statespec::test::require(
        resolved.workers.size() == 1, "semantic resolver should preserve invalid worker"
    );
    statespec::test::require(
        resolved.workers[0].polls.has_value(), "semantic resolver should preserve reference text"
    );
    statespec::test::require(
        resolved.workers[0].polls->name == "MissingQueue.MissingMessage",
        "semantic resolver should preserve unresolved reference name"
    );
    statespec::test::require(
        !resolved.workers[0].polls->resolved(),
        "semantic resolver should mark missing reference unresolved"
    );
}

void semantic_resolver_resolves_entity_relationship_references()
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
              initial Pending
            }
          }
        }
    )sspec");

    const auto resolved = statespec::resolve_semantics(spec);

    statespec::test::require(
        resolved.entities.size() == 2, "semantic resolver should lower entities"
    );
    statespec::test::require(
        resolved.entities[0].children.size() == 1, "semantic resolver should lower children"
    );
    statespec::test::require(
        resolved.entities[0].children[0].target_entity.kind == statespec::SymbolKind::Entity,
        "semantic resolver should resolve child target entity"
    );
    statespec::test::require(
        resolved.entities[1].relations.size() == 1, "semantic resolver should lower relations"
    );
    statespec::test::require(
        resolved.entities[1].relations[0].target.kind == statespec::SymbolKind::Entity,
        "semantic resolver should resolve relation target entity"
    );
    statespec::test::require(
        resolved.entities[1].ownership.has_value(), "semantic resolver should lower ownership"
    );
    statespec::test::require(
        resolved.entities[1].invariants.size() == 1, "semantic resolver should lower invariants"
    );
}

} // namespace

TEST_CASE("semantic resolver resolves runtime references")
{
    semantic_resolver_resolves_runtime_references();
}

TEST_CASE("semantic resolver preserves unresolved references")
{
    semantic_resolver_preserves_unresolved_references();
}

TEST_CASE("semantic resolver resolves entity relationship references")
{
    semantic_resolver_resolves_entity_relationship_references();
}
