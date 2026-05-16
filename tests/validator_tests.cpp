#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/validator.hpp"

namespace
{

statespec::DiagnosticBag validate_text(const std::string& text)
{
    statespec::DiagnosticBag diagnostics;
    const auto spec = statespec::test::parse_text(text, diagnostics);
    if (!diagnostics.has_errors())
    {
        statespec::Validator validator;
        validator.validate(spec, diagnostics);
    }
    return diagnostics;
}

void validator_rejects_missing_system()
{
    statespec::Spec spec;
    statespec::DiagnosticBag diagnostics;
    statespec::Validator validator;
    validator.validate(spec, diagnostics);
    statespec::test::require(
        diagnostics.has_errors(), "validator should reject spec without system"
    );
}

void validator_accepts_entity_relationship_model()
{
    const auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Account {
            key account_id
            children {
              orders: Order by account_id
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              account_id string
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
              created_at timestamp
              updated_at timestamp
              status string
              order_id string
              account_id string
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

    statespec::test::require(
        !diagnostics.has_errors(), "validator should accept valid entity relationships"
    );
}

void validator_rejects_invalid_entity_relationship_model()
{
    const auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Account {
            key account_id
            children {
              orders: Order by missing_account_id
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
              authority: local
              system_of_record: self
              lifecycle: temporary
            }
            relations {
              parent account_id: ref<MissingAccount> {
                kind: ownership
                on_parent_delete: detach
                parent_must_be_in: [MissingState]
                unique_within_parent: [missing_field]
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
              valid_status: status != ""
            }
            state_machine {
              state Pending
              initial Pending
            }
          }
        }
    )sspec");

    statespec::test::require(
        diagnostics.has_errors(), "validator should reject invalid entity relationships"
    );
}

} // namespace

TEST_CASE("validator rejects missing systems")
{
    validator_rejects_missing_system();
}

TEST_CASE("validator accepts entity relationship model")
{
    validator_accepts_entity_relationship_model();
}

TEST_CASE("validator rejects invalid entity relationship model")
{
    validator_rejects_invalid_entity_relationship_model();
}
