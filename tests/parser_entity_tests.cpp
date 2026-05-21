#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"

namespace
{

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
} // namespace

TEST_CASE("parser parses entity fields, indexes, and state machines")
{
    parser_parses_entity_fields_and_state_machine();
}

TEST_CASE("parser parses entity ownership, relations, children, and invariants")
{
    parser_parses_entity_ownership_relations_children_and_invariants();
}
