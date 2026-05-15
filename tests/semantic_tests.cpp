#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/semantic.hpp"

namespace
{

void semantic_resolver_resolves_runtime_references()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
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
            path "/v1/orders/{order_id}/start"
            starts workflow OrderProcessing
            enqueues EmailDispatch.SendConfirmation
          }

          workflow OrderProcessing {
            version 1
            singleton false
            expected_execution_time PT60S
            start validate_order
            step validate_order {
              expected_execution_time PT10S
              max_retries 1
            }
          }

          policy OrderAccess {
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

    const auto& api = resolved.apis[0];
    statespec::test::require(
        api.starts_workflow.has_value() &&
            api.starts_workflow->kind == statespec::SymbolKind::Workflow,
        "semantic resolver should resolve API workflow target"
    );
    statespec::test::require(
        api.enqueues.has_value() && api.enqueues->kind == statespec::SymbolKind::Message,
        "semantic resolver should resolve API queue message target"
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

} // namespace

TEST_CASE("semantic resolver resolves runtime references")
{
    semantic_resolver_resolves_runtime_references();
}

TEST_CASE("semantic resolver preserves unresolved references")
{
    semantic_resolver_preserves_unresolved_references();
}
