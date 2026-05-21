#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"

namespace
{

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
} // namespace

TEST_CASE("parser parses queues and messages")
{
    parser_parses_queue_and_message();
}

TEST_CASE("parser parses leases and workers")
{
    parser_parses_lease_and_worker();
}
