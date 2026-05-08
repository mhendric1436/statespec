#include "statespec/diagnostic.hpp"
#include "statespec/lexer.hpp"
#include "statespec/parser.hpp"
#include "statespec/source.hpp"
#include "statespec/validator.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

namespace
{

void require(
    bool condition,
    const char* message
)
{
    if (!condition)
    {
        std::cerr << "test failed: " << message << '\n';
        std::exit(1);
    }
}

statespec::DiagnosticBag validate_text(const std::string& text)
{
    statespec::DiagnosticBag diagnostics;
    statespec::SourceFile source{"validator_test.sspec", text};
    statespec::Lexer lexer{source};
    auto tokens = lexer.lex(diagnostics);
    statespec::Parser parser{std::move(tokens)};
    auto spec = parser.parse(diagnostics);
    statespec::Validator validator;
    validator.validate(spec, diagnostics);
    return diagnostics;
}

bool has_error_code(
    const statespec::DiagnosticBag& diagnostics,
    const std::string& code
)
{
    for (const auto& diagnostic : diagnostics.all())
    {
        if (diagnostic.severity == statespec::DiagnosticSeverity::Error && diagnostic.code == code)
        {
            return true;
        }
    }
    return false;
}

void validator_accepts_resolved_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key order_id
            fields {
              order_id string
              status string
            }
          }
          queue EmailQueue {
            namespace workflow_ns
            channel email
            message SendConfirmation {
              idempotency_key message_id
              payload {
                message_id string
                order_id string
              }
            }
          }
          lease WorkerLease {
            resource "worker"
            ttl PT30S
          }
          workflow OrderProcessing {
            version 1
            start validate_order
            step validate_order {
              expected_execution_time PT10S
              max_retries 1
            }
          }
          worker OrderWorker {
            singleton true
            lease WorkerLease
            polls EmailQueue.SendConfirmation
            executes OrderProcessing
          }
          api StartOrderProcessing {
            method POST
            path "/v1/orders/start"
            starts workflow OrderProcessing
            enqueues EmailQueue.SendConfirmation
          }
          policy WorkflowAccess {
            allow StartOrderProcessing when caller.role == operator;
            audit StartOrderProcessing;
          }
          generate mt
          generate wf
        }
    )sspec");

    require(!diagnostics.has_errors(), "validator should accept resolved references");
}

void validator_rejects_duplicate_top_level_names()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key id
            fields { id string }
          }
          entity Order {
            key id
            fields { id string }
          }
        }
    )sspec");

    require(has_error_code(diagnostics, "SSPEC3001"), "validator should reject duplicate top-level names");
}

void validator_rejects_missing_entity_key_field()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          entity Order {
            key order_id
            fields { status string }
          }
        }
    )sspec");

    require(has_error_code(diagnostics, "SSPEC3002"), "validator should reject unknown entity key field");
}

void validator_rejects_unknown_workflow_start_step()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          workflow OrderProcessing {
            version 1
            start missing_step
            step validate_order {
              expected_execution_time PT10S
            }
          }
        }
    )sspec");

    require(has_error_code(diagnostics, "SSPEC3002"), "validator should reject unknown workflow start step");
}

void validator_rejects_unknown_worker_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          worker WorkerA {
            singleton true
            lease MissingLease
            polls MissingQueue.Message
            executes MissingWorkflow
          }
        }
    )sspec");

    require(has_error_code(diagnostics, "SSPEC3002"), "validator should reject unknown worker references");
}

void validator_rejects_unknown_api_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          api StartOrderProcessing {
            method POST
            path "/v1/orders/start"
            starts workflow MissingWorkflow
            enqueues MissingQueue.Message
          }
        }
    )sspec");

    require(has_error_code(diagnostics, "SSPEC3002"), "validator should reject unknown API references");
}

void validator_rejects_invalid_generate_target()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          generate unknown_target
        }
    )sspec");

    require(has_error_code(diagnostics, "SSPEC3002"), "validator should reject invalid generate target");
}

} // namespace

int main()
{
    validator_accepts_resolved_references();
    validator_rejects_duplicate_top_level_names();
    validator_rejects_missing_entity_key_field();
    validator_rejects_unknown_workflow_start_step();
    validator_rejects_unknown_worker_references();
    validator_rejects_unknown_api_references();
    validator_rejects_invalid_generate_target();

    std::cout << "validator milestone tests passed\n";
    return 0;
}
