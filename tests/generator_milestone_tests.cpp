#include "statespec/diagnostic.hpp"
#include "statespec/generator.hpp"
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

statespec::Spec parse_and_validate(
    const std::string& text,
    statespec::DiagnosticBag& diagnostics
)
{
    statespec::SourceFile source{"generator_test.sspec", text};
    statespec::Lexer lexer{source};
    auto tokens = lexer.lex(diagnostics);

    statespec::Parser parser{std::move(tokens)};
    auto spec = parser.parse(diagnostics);

    statespec::Validator validator;
    validator.validate(spec, diagnostics);
    return spec;
}

std::string generator_fixture()
{
    return R"sspec(
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
          generate mt { out "generated/mt" }
          generate dl { out "generated/dl" }
          generate qu { out "generated/qu" }
          generate wf { out "generated/wf" }
          generate openapi { out "generated/openapi" }
        }
    )sspec";
}

bool has_file(
    const statespec::GenerationResult& result,
    const std::string& path
)
{
    for (const auto& file : result.files)
    {
        if (file.path == path)
        {
            return true;
        }
    }
    return false;
}

const statespec::GeneratedFile& find_file(
    const statespec::GenerationResult& result,
    const std::string& path
)
{
    for (const auto& file : result.files)
    {
        if (file.path == path)
        {
            return file;
        }
    }

    std::cerr << "test failed: generated file not found: " << path << '\n';
    std::exit(1);
}

void generator_emits_files_for_generate_declarations()
{
    statespec::DiagnosticBag diagnostics;
    const auto spec = parse_and_validate(generator_fixture(), diagnostics);
    require(!diagnostics.has_errors(), "generator fixture should parse and validate");

    statespec::Generator generator;
    statespec::GenerationOptions options;
    const auto result = generator.generate(spec, options, diagnostics);

    require(!diagnostics.has_errors(), "generator should not emit diagnostics for valid fixture");
    require(has_file(result, "generated/mt/mt-manifest.yaml"), "generator should emit mt manifest");
    require(has_file(result, "generated/dl/dl-manifest.yaml"), "generator should emit dl manifest");
    require(has_file(result, "generated/qu/qu-manifest.yaml"), "generator should emit qu manifest");
    require(has_file(result, "generated/wf/wf-manifest.yaml"), "generator should emit wf manifest");
    require(has_file(result, "generated/openapi/openapi.yaml"), "generator should emit openapi stub");
}

void generator_target_override_selects_one_target()
{
    statespec::DiagnosticBag diagnostics;
    const auto spec = parse_and_validate(generator_fixture(), diagnostics);
    require(!diagnostics.has_errors(), "generator fixture should parse and validate");

    statespec::Generator generator;
    statespec::GenerationOptions options;
    options.target_override = "wf";
    const auto result = generator.generate(spec, options, diagnostics);

    require(!diagnostics.has_errors(), "generator target override should not fail");
    require(result.files.size() == 1, "generator target override should emit one file");
    require(result.files[0].path == "generated/wf/wf-manifest.yaml", "generator target override should emit wf manifest");
    require(result.files[0].content.find("OrderProcessing") != std::string::npos, "wf manifest should include workflow name");
}

void generator_out_override_changes_output_root()
{
    statespec::DiagnosticBag diagnostics;
    const auto spec = parse_and_validate(generator_fixture(), diagnostics);
    require(!diagnostics.has_errors(), "generator fixture should parse and validate");

    statespec::Generator generator;
    statespec::GenerationOptions options;
    options.target_override = "mt";
    options.out_override = "tmp/generated";
    const auto result = generator.generate(spec, options, diagnostics);

    require(!diagnostics.has_errors(), "generator output override should not fail");
    require(result.files.size() == 1, "generator output override should emit one file");
    require(result.files[0].path == "tmp/generated/mt-manifest.yaml", "generator output override should change root");
}

void generator_emits_scaffold_content()
{
    statespec::DiagnosticBag diagnostics;
    const auto spec = parse_and_validate(generator_fixture(), diagnostics);
    require(!diagnostics.has_errors(), "generator fixture should parse and validate");

    statespec::Generator generator;
    statespec::GenerationOptions options;
    const auto result = generator.generate(spec, options, diagnostics);

    const auto& mt = find_file(result, "generated/mt/mt-manifest.yaml");
    const auto& qu = find_file(result, "generated/qu/qu-manifest.yaml");
    const auto& openapi = find_file(result, "generated/openapi/openapi.yaml");

    require(mt.content.find("Order") != std::string::npos, "mt manifest should include entity name");
    require(qu.content.find("EmailQueue") != std::string::npos, "qu manifest should include queue name");
    require(openapi.content.find("openapi: 3.1.0") != std::string::npos, "openapi stub should include version header");
}

} // namespace

void run_generator_milestone_tests()
{
    generator_emits_files_for_generate_declarations();
    generator_target_override_selects_one_target();
    generator_out_override_changes_output_root();
    generator_emits_scaffold_content();
}

namespace
{

const bool generator_milestone_tests_ran = []() {
    run_generator_milestone_tests();
    return true;
}();

} // namespace
