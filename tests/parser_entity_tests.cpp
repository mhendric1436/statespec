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

void parser_parses_entity_owned_crud_api_block()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system AccountSystem {
          entity Account {
            key tenant_id, account_id
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              account_id string
              display_name string
            }
            indexes {
              index by_account_status on account_id, status
            }
            state_machine {
              state Active
              initial Active
            }
            api {
              resource "/v1/tenants/{tenant_id}/accounts/{account_id}"
              create CreateAccount {
                fields [display_name]
              }
              get GetAccount
              list {
                path "/v1/tenants/{tenant_id}/accounts"
                by tenant_id
              }
              list AccountProjects {
                path "/v1/tenants/{tenant_id}/accounts/{account_id}/projects"
                by by_account_status
              }
              update_status UpdateAccountStatus
              delete DeleteAccount
            }
          }
        }
    )sspec");

    statespec::test::require(spec.system.has_value(), "parser should parse system");
    statespec::test::require(spec.system->entities.size() == 1, "parser should parse entity");
    const auto& entity = spec.system->entities[0];
    statespec::test::require(entity.api.has_value(), "parser should parse entity api block");
    statespec::test::require(
        entity.api->resource == "/v1/tenants/{tenant_id}/accounts/{account_id}",
        "parser should parse entity api resource"
    );
    statespec::test::require(entity.api->create.has_value(), "parser should parse create intent");
    statespec::test::require(
        entity.api->create->name == "CreateAccount", "parser should parse create override name"
    );
    statespec::test::require(
        entity.api->create->fields.size() == 1 && entity.api->create->fields[0] == "display_name",
        "parser should parse create fields"
    );
    statespec::test::require(entity.api->get.has_value(), "parser should parse get intent");
    statespec::test::require(
        entity.api->get->name == "GetAccount", "parser should parse get override name"
    );
    statespec::test::require(entity.api->lists.size() == 2, "parser should parse list intents");
    statespec::test::require(
        !entity.api->lists[0].name.has_value(), "parser should parse anonymous list"
    );
    statespec::test::require(
        entity.api->lists[0].by.size() == 1 && entity.api->lists[0].by[0] == "tenant_id",
        "parser should parse list field selector"
    );
    statespec::test::require(
        entity.api->lists[1].name == "AccountProjects", "parser should parse named list"
    );
    statespec::test::require(
        entity.api->lists[1].by.size() == 1 && entity.api->lists[1].by[0] == "by_account_status",
        "parser should parse list index selector"
    );
    statespec::test::require(
        entity.api->update_status.has_value(), "parser should parse update_status intent"
    );
    statespec::test::require(
        entity.api->update_status->name == "UpdateAccountStatus",
        "parser should parse update_status override name"
    );
    statespec::test::require(entity.api->delete_.has_value(), "parser should parse delete intent");
    statespec::test::require(
        entity.api->delete_->name == "DeleteAccount", "parser should parse delete override name"
    );
}

void parser_does_not_treat_next_crud_operation_as_override_name()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system AccountSystem {
          entity Account {
            key tenant_id, account_id
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              account_id string
            }
            state_machine {
              state Active
              state Deleted {
                terminal: true
              }
              initial Active
              terminal [Deleted]
              Active -> Deleted
            }
            api {
              resource "/v1/tenants/{tenant_id}/accounts/{account_id}"
              create
              get
              list {
                path "/v1/tenants/{tenant_id}/accounts"
                by tenant_id
              }
              update_status
              delete
            }
          }
        }
    )sspec");

    const auto& api = spec.system->entities[0].api;
    statespec::test::require(api.has_value(), "parser should parse entity api block");
    statespec::test::require(api->create.has_value(), "parser should parse create intent");
    statespec::test::require(
        !api->create->name.has_value(), "parser should leave create name unset"
    );
    statespec::test::require(api->get.has_value(), "parser should parse get intent");
    statespec::test::require(!api->get->name.has_value(), "parser should leave get name unset");
    statespec::test::require(api->lists.size() == 1, "parser should parse list intent");
    statespec::test::require(
        !api->lists[0].name.has_value(), "parser should leave list name unset"
    );
    statespec::test::require(
        api->update_status.has_value(), "parser should parse update_status intent"
    );
    statespec::test::require(
        !api->update_status->name.has_value(), "parser should leave update_status name unset"
    );
    statespec::test::require(api->delete_.has_value(), "parser should parse delete intent");
    statespec::test::require(
        !api->delete_->name.has_value(), "parser should leave delete name unset"
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

TEST_CASE("parser parses entity-owned CRUD API block")
{
    parser_parses_entity_owned_crud_api_block();
}

TEST_CASE("parser does not treat next CRUD operation as override name")
{
    parser_does_not_treat_next_crud_operation_as_override_name();
}
