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

void ir_lowers_entity_api_intent_to_synthetic_api_contracts()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system AccountSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          entity Account {
            key tenant_id, account_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              account_id string
              display_name string
            }
            indexes {
              index by_tenant_status on tenant_id, status
            }
            state_machine {
              state Active
              state Deleted {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Active
              terminal [Deleted]
              Active -> Deleted
            }
            api {
              resource "/v1/tenants/{tenant_id}/accounts/{account_id}"
              create {
                fields [display_name]
              }
              get
              list {
                path "/v1/tenants/{tenant_id}/accounts"
                by tenant_id
              }
              list ListAccountsByStatus {
                path "/v1/tenants/{tenant_id}/accounts/by-status"
                by by_tenant_status
              }
              update_status
              delete
            }
          }
        }
    )sspec");

    const auto ir = statespec::lower_to_ir(spec);

    statespec::test::require(ir.api_servers.size() == 1, "IR should synthesize API server");
    statespec::test::require(
        ir.api_servers[0].name == "EntityApi", "IR should name synthetic API server"
    );
    statespec::test::require(ir.apis.size() == 6, "IR should synthesize CRUD APIs");

    const auto& create = ir.apis[0];
    statespec::test::require(create.name == "CreateAccount", "IR should name create API");
    statespec::test::require(create.method == "POST", "IR should lower create method");
    statespec::test::require(
        create.input == "CreateAccountRequest", "IR should link create request shape"
    );
    statespec::test::require(
        create.output == "AccountResponse", "IR should link entity response shape"
    );
    statespec::test::require(
        create.entity == "Account" && create.repository_operation == "create",
        "IR should link create API to repository operation"
    );

    const auto& list = ir.apis[2];
    statespec::test::require(list.name == "ListAccounts", "IR should name default list API");
    statespec::test::require(
        list.output == "ListAccountsResponse", "IR should link list response shape"
    );
    statespec::test::require(
        list.list_selector.size() == 1 && list.list_selector[0] == "tenant_id",
        "IR should lower list selector"
    );

    const auto& indexed_list = ir.apis[3];
    statespec::test::require(
        indexed_list.name == "ListAccountsByStatus", "IR should honor list override name"
    );
    statespec::test::require(
        indexed_list.list_selector.size() == 2 && indexed_list.list_selector[0] == "tenant_id" &&
            indexed_list.list_selector[1] == "status",
        "IR should lower index-name selector to index fields"
    );

    const auto& update = ir.apis[4];
    statespec::test::require(
        update.name == "UpdateAccountStatus", "IR should name update_status API"
    );
    statespec::test::require(
        update.path == "/v1/tenants/{tenant_id}/accounts/{account_id}/status",
        "IR should derive update_status path"
    );

    bool found_account_response = false;
    bool found_create_request = false;
    bool found_list_response = false;
    for (const auto& shape : ir.shapes)
    {
        found_account_response = found_account_response || shape.name == "AccountResponse";
        found_create_request = found_create_request || shape.name == "CreateAccountRequest";
        found_list_response = found_list_response || shape.name == "ListAccountsResponse";
    }
    statespec::test::require(found_account_response, "IR should synthesize response shape");
    statespec::test::require(found_create_request, "IR should synthesize create request shape");
    statespec::test::require(found_list_response, "IR should synthesize list response shape");
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

TEST_CASE("IR lowers entity API intent to synthetic API contracts")
{
    ir_lowers_entity_api_intent_to_synthetic_api_contracts();
}
