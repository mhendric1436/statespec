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
            renew_every PT10S
            holder worker_id
            fencing_token true
            max_ttl PT5M
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
            concurrency 4
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
    require(
        has_file(result, "generated/mt/mt_entities.hpp"), "generator should emit mt entities header"
    );
    require(
        has_file(result, "generated/mt/mt_metadata.cpp"), "generator should emit mt metadata source"
    );
    require(
        has_file(result, "generated/mt/mt-state-machines.yaml"),
        "generator should emit mt state-machine manifest"
    );
    require(has_file(result, "generated/dl/dl-manifest.yaml"), "generator should emit dl manifest");
    require(
        has_file(result, "generated/dl/dl_leases.hpp"), "generator should emit dl leases header"
    );
    require(
        has_file(result, "generated/dl/dl_metadata.cpp"), "generator should emit dl metadata source"
    );
    require(has_file(result, "generated/qu/qu-manifest.yaml"), "generator should emit qu manifest");
    require(
        has_file(result, "generated/qu/qu_messages.hpp"), "generator should emit qu messages header"
    );
    require(
        has_file(result, "generated/qu/qu_metadata.cpp"), "generator should emit qu metadata source"
    );
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
    options.target_override = "dl";
    const auto result = generator.generate(spec, options, diagnostics);

    require(!diagnostics.has_errors(), "generator target override should not fail");
    require(result.files.size() == 3, "dl target override should emit dl file family");
    require(
        has_file(result, "generated/dl/dl-manifest.yaml"), "dl target override should emit manifest"
    );
    require(
        has_file(result, "generated/dl/dl_leases.hpp"), "dl target override should emit header"
    );
    require(
        has_file(result, "generated/dl/dl_metadata.cpp"), "dl target override should emit source"
    );
}

void generator_out_override_changes_output_root()
{
    statespec::DiagnosticBag diagnostics;
    const auto spec = parse_and_validate(generator_fixture(), diagnostics);
    require(!diagnostics.has_errors(), "generator fixture should parse and validate");

    statespec::Generator generator;
    statespec::GenerationOptions options;
    options.target_override = "dl";
    options.out_override = "tmp/generated";
    const auto result = generator.generate(spec, options, diagnostics);

    require(!diagnostics.has_errors(), "generator output override should not fail");
    require(result.files.size() == 3, "generator output override should emit dl file family");
    require(
        has_file(result, "tmp/generated/dl-manifest.yaml"),
        "generator output override should change manifest root"
    );
    require(
        has_file(result, "tmp/generated/dl_leases.hpp"),
        "generator output override should change header root"
    );
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
    const auto& dl = find_file(result, "generated/dl/dl-manifest.yaml");
    const auto& dl_header = find_file(result, "generated/dl/dl_leases.hpp");
    const auto& dl_source = find_file(result, "generated/dl/dl_metadata.cpp");
    const auto& qu = find_file(result, "generated/qu/qu-manifest.yaml");
    const auto& qu_header = find_file(result, "generated/qu/qu_messages.hpp");
    const auto& qu_source = find_file(result, "generated/qu/qu_metadata.cpp");
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
        dl.content.find("fencing_token: true") != std::string::npos,
        "dl manifest should include fencing token metadata"
    );
    require(
        dl_header.content.find("struct LeaseMetadata") != std::string::npos,
        "dl header should include lease metadata"
    );
    require(
        dl_header.content.find("struct WorkerLeaseBinding") != std::string::npos,
        "dl header should include worker lease bindings"
    );
    require(
        dl_source.content.find("find_lease") != std::string::npos,
        "dl source should include lease lookup"
    );
    require(
        qu.content.find("payload_struct: EmailQueueSendConfirmationPayload") != std::string::npos,
        "qu manifest should include payload struct name"
    );
    require(
        qu_header.content.find("struct EmailQueueSendConfirmationPayload") != std::string::npos,
        "qu header should include message payload struct"
    );
    require(
        qu_header.content.find("std::string message_id") != std::string::npos,
        "qu header should include payload fields"
    );
    require(
        qu_source.content.find("idempotency_key_field") != std::string::npos,
        "qu source should include idempotency key lookup"
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
