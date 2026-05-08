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
              updated_at timestamp?
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
    require(has_file(result, "generated/mt/mt_entities.hpp"), "generator should emit mt entities header");
    require(has_file(result, "generated/mt/mt_metadata.cpp"), "generator should emit mt metadata source");
    require(has_file(result, "generated/mt/mt-state-machines.yaml"), "generator should emit mt state-machine manifest");
    require(has_file(result, "generated/dl/dl-manifest.yaml"), "generator should emit dl manifest");
    require(has_file(result, "generated/qu/qu-manifest.yaml"), "generator should emit qu manifest");
    require(has_file(result, "generated/wf/wf-manifest.yaml"), "generator should emit wf manifest");
    require(
        has_file(result, "generated/openapi/openapi.yaml"), "generator should emit openapi stub"
    );
}

void generator_target_override_selects_one_target_family()
{
    statespec::DiagnosticBag diagnostics;
    const auto spec = parse_and_validate(generator_fixture(), diagnostics);
    require(!diagnostics.has_errors(), "generator fixture should parse and validate");

    statespec::Generator generator;
    statespec::GenerationOptions options;
    options.target_override = "mt";
    const auto result = generator.generate(spec, options, diagnostics);

    require(!diagnostics.has_errors(), "generator target override should not fail");
    require(result.files.size() == 4, "mt target override should emit mt file family");
    require(has_file(result, "generated/mt/mt-manifest.yaml"), "mt target override should emit manifest");
    require(has_file(result, "generated/mt/mt_entities.hpp"), "mt target override should emit header");
    require(has_file(result, "generated/mt/mt_metadata.cpp"), "mt target override should emit source");
    require(has_file(result, "generated/mt/mt-state-machines.yaml"), "mt target override should emit state metadata");
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
    require(result.files.size() == 4, "generator output override should emit mt file family");
    require(has_file(result, "tmp/generated/mt-manifest.yaml"), "generator output override should change manifest root");
    require(has_file(result, "tmp/generated/mt_entities.hpp"), "generator output override should change header root");
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
    const auto& mt_header = find_file(result, "generated/mt/mt_entities.hpp");
    const auto& mt_source = find_file(result, "generated/mt/mt_metadata.cpp");
    const auto& qu = find_file(result, "generated/qu/qu-manifest.yaml");
    const auto& openapi = find_file(result, "generated/openapi/openapi.yaml");

    require(
        mt.content.find("cpp_type: std::optional<std::chrono::system_clock::time_point>") !=
            std::string::npos,
        "mt manifest should include optional timestamp C++ type"
    );
    require(
        mt_header.content.find("struct Order") != std::string::npos,
        "mt header should include entity struct"
    );
    require(
        mt_header.content.find("std::optional<std::chrono::system_clock::time_point> updated_at") !=
            std::string::npos,
        "mt header should map timestamp? to optional chrono time point"
    );
    require(
        mt_source.content.find("EntityMetadata") != std::string::npos,
        "mt source should include entity metadata"
    );
    require(
        qu.content.find("EmailQueue") != std::string::npos, "qu manifest should include queue name"
    );
    require(
        openapi.content.find("openapi: 3.1.0") != std::string::npos,
        "openapi stub should include version header"
    );
}

} // namespace

void run_generator_milestone_tests()
{
    generator_emits_files_for_generate_declarations();
    generator_target_override_selects_one_target_family();
    generator_out_override_changes_output_root();
    generator_emits_scaffold_content();
}

namespace
{

const bool generator_milestone_tests_ran = []()
{
    run_generator_milestone_tests();
    return true;
}();

} // namespace
