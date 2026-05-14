#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/ir.hpp"

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

} // namespace

TEST_CASE("IR lowers terminal garbage collection policy")
{
    ir_lowers_terminal_garbage_collection_policy();
}
