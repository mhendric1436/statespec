#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/ir.hpp"
#include "statespec/semantic.hpp"

namespace
{

void ir_lowers_terminal_garbage_collection_policy()
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
            state_machine {
              state Creating
              state Deleted {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Creating
              terminal [Deleted]
              Creating -> Deleted
            }
          }
        }
    )sspec");

    const auto ir = statespec::lower_to_ir(spec);

    statespec::test::require(ir.entities.size() == 1, "IR should lower entity");
    const auto& entity = ir.entities[0];
    statespec::test::require(entity.states.size() == 2, "IR should lower entity states");
    statespec::test::require(entity.initial_state == "Creating", "IR should lower initial state");
    statespec::test::require(
        entity.terminal_states.size() == 1, "IR should lower terminal state list"
    );

    const auto& deleted = entity.states[1];
    statespec::test::require(deleted.name == "Deleted", "IR should lower state name");
    statespec::test::require(deleted.terminal, "IR should lower terminal flag");
    statespec::test::require(
        deleted.garbage_collection.has_value(), "IR should lower garbage collection policy"
    );
    statespec::test::require(
        deleted.garbage_collection->after == "P30D", "IR should lower garbage collection duration"
    );
    statespec::test::require(
        deleted.garbage_collection->mode == "tombstone", "IR should lower garbage collection mode"
    );
}

void ir_lowers_entity_relationship_metadata()
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
              initial Pending
            }
          }
        }
    )sspec");

    const auto resolved = statespec::resolve_semantics(spec);
    const auto ir = statespec::lower_to_ir(resolved);

    statespec::test::require(ir.entities.size() == 2, "IR should lower entities");
    statespec::test::require(ir.entities[0].children.size() == 1, "IR should lower children");
    statespec::test::require(
        ir.entities[0].children[0].target_entity == "Order", "IR should lower child target"
    );

    const auto& order = ir.entities[1];
    statespec::test::require(order.ownership.has_value(), "IR should lower ownership");
    statespec::test::require(
        order.ownership->authority == "system", "IR should lower ownership authority"
    );
    statespec::test::require(order.relations.size() == 1, "IR should lower relations");
    statespec::test::require(
        order.relations[0].target == "Account", "IR should lower resolved relation target"
    );
    statespec::test::require(
        order.relations[0].relation_kind == "composition", "IR should lower relation options"
    );
    statespec::test::require(order.invariants.size() == 1, "IR should lower invariants");
    statespec::test::require(
        order.invariants[0].name == "valid_status", "IR should lower invariant name"
    );
}
} // namespace

TEST_CASE("IR lowers terminal garbage collection policy")
{
    ir_lowers_terminal_garbage_collection_policy();
}

TEST_CASE("IR lowers entity relationship metadata")
{
    ir_lowers_entity_relationship_metadata();
}
