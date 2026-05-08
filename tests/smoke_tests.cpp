#include "statespec/diagnostic.hpp"
#include "statespec/lexer.hpp"
#include "statespec/parser.hpp"
#include "statespec/source.hpp"
#include "statespec/symbol_table.hpp"
#include "statespec/token.hpp"
#include "statespec/validator.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{

void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "test failed: " << message << '\n';
        std::exit(1);
    }
}

std::vector<statespec::Token> lex_text(
    const std::string& text,
    statespec::DiagnosticBag& diagnostics
)
{
    statespec::SourceFile source{"test.sspec", text};
    statespec::Lexer lexer{source};
    return lexer.lex(diagnostics);
}

std::vector<statespec::Token> lex_text(const std::string& text)
{
    statespec::DiagnosticBag diagnostics;
    auto tokens = lex_text(text, diagnostics);
    require(!diagnostics.has_errors(), "lexer should not produce diagnostics for valid input");
    return tokens;
}

statespec::Spec parse_text(
    const std::string& text,
    statespec::DiagnosticBag& diagnostics
)
{
    auto tokens = lex_text(text, diagnostics);
    statespec::Parser parser{std::move(tokens)};
    return parser.parse(diagnostics);
}

statespec::Spec parse_text(const std::string& text)
{
    statespec::DiagnosticBag diagnostics;
    auto spec = parse_text(text, diagnostics);
    require(!diagnostics.has_errors(), "parser should not produce errors for valid input");
    return spec;
}

void diagnostic_bag_records_errors()
{
    statespec::DiagnosticBag diagnostics;
    require(!diagnostics.has_errors(), "new DiagnosticBag should not have errors");
    diagnostics.error(statespec::SourceRange{}, "TEST", "example");
    require(diagnostics.has_errors(), "DiagnosticBag should report errors");
    require(diagnostics.all().size() == 1, "DiagnosticBag should store diagnostics");
}

void symbol_table_rejects_duplicates()
{
    statespec::SymbolTable table;
    require(
        table.insert(statespec::Symbol{statespec::SymbolKind::Entity, "Order", {}}),
        "first symbol insert should succeed"
    );
    require(
        !table.insert(statespec::Symbol{statespec::SymbolKind::Entity, "Order", {}}),
        "duplicate symbol insert should fail"
    );
    require(table.find("Order").has_value(), "inserted symbol should be findable");
}

void lexer_emits_eof_for_empty_source()
{
    const auto tokens = lex_text("");
    require(tokens.size() == 1, "empty source should produce one EOF token");
    require(tokens[0].kind == statespec::TokenKind::EndOfFile, "empty source token should be EOF");
}

void lexer_tokenizes_system_declaration()
{
    const auto tokens = lex_text("statespec 0.1; system OrderSystem {}");
    require(
        tokens.size() == 8, "system declaration should produce expected token count including EOF"
    );
    require(
        tokens[0].kind == statespec::TokenKind::KeywordStatespec, "statespec should be a keyword"
    );
    require(
        tokens[1].kind == statespec::TokenKind::DecimalLiteral, "0.1 should be a decimal literal"
    );
    require(tokens[2].kind == statespec::TokenKind::Semicolon, "semicolon should be tokenized");
    require(tokens[3].kind == statespec::TokenKind::KeywordSystem, "system should be a keyword");
    require(tokens[4].kind == statespec::TokenKind::Identifier, "system name should be identifier");
    require(tokens[4].lexeme == "OrderSystem", "identifier lexeme should be preserved");
    require(tokens[5].kind == statespec::TokenKind::LeftBrace, "left brace should be tokenized");
    require(tokens[6].kind == statespec::TokenKind::RightBrace, "right brace should be tokenized");
    require(tokens[7].kind == statespec::TokenKind::EndOfFile, "last token should be EOF");
}

void lexer_skips_comments()
{
    const auto tokens = lex_text("// line comment\nsystem A { /* block comment */ entity B {} }");
    require(
        tokens[0].kind == statespec::TokenKind::KeywordSystem, "line comment should be skipped"
    );
    require(tokens[1].lexeme == "A", "identifier after comment should be tokenized");
    require(
        tokens[3].kind == statespec::TokenKind::KeywordEntity, "block comment should be skipped"
    );
}

void lexer_tokenizes_literals()
{
    const auto tokens = lex_text("true false 42 7.5 \"hello\\nworld\" PT30S P1DT2H3M4S");
    require(
        tokens[0].kind == statespec::TokenKind::BooleanLiteral, "true should be boolean literal"
    );
    require(
        tokens[1].kind == statespec::TokenKind::BooleanLiteral, "false should be boolean literal"
    );
    require(tokens[2].kind == statespec::TokenKind::IntegerLiteral, "42 should be integer literal");
    require(
        tokens[3].kind == statespec::TokenKind::DecimalLiteral, "7.5 should be decimal literal"
    );
    require(
        tokens[4].kind == statespec::TokenKind::StringLiteral,
        "quoted string should be string literal"
    );
    require(
        tokens[5].kind == statespec::TokenKind::DurationLiteral, "PT30S should be duration literal"
    );
    require(
        tokens[6].kind == statespec::TokenKind::DurationLiteral,
        "P1DT2H3M4S should be duration literal"
    );
}

void lexer_tokenizes_operators_and_punctuation()
{
    const auto tokens = lex_text("{}()[]<>,:;.? -> = == != <= >= + - * / % ! && ||");
    const std::vector<statespec::TokenKind> expected{
        statespec::TokenKind::LeftBrace,    statespec::TokenKind::RightBrace,
        statespec::TokenKind::LeftParen,    statespec::TokenKind::RightParen,
        statespec::TokenKind::LeftBracket,  statespec::TokenKind::RightBracket,
        statespec::TokenKind::Less,         statespec::TokenKind::Greater,
        statespec::TokenKind::Comma,        statespec::TokenKind::Colon,
        statespec::TokenKind::Semicolon,    statespec::TokenKind::Dot,
        statespec::TokenKind::Question,     statespec::TokenKind::Arrow,
        statespec::TokenKind::Equals,       statespec::TokenKind::EqualEqual,
        statespec::TokenKind::BangEqual,    statespec::TokenKind::LessEqual,
        statespec::TokenKind::GreaterEqual, statespec::TokenKind::Plus,
        statespec::TokenKind::Minus,        statespec::TokenKind::Star,
        statespec::TokenKind::Slash,        statespec::TokenKind::Percent,
        statespec::TokenKind::Bang,         statespec::TokenKind::AndAnd,
        statespec::TokenKind::OrOr,         statespec::TokenKind::EndOfFile,
    };

    require(tokens.size() == expected.size(), "operator token count should match expected count");
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        require(
            tokens[i].kind == expected[i], "operator token kind should match expected sequence"
        );
    }
}

void lexer_tracks_line_and_column()
{
    const auto tokens = lex_text("system\n  OrderSystem");
    require(tokens[0].range.begin.line == 1, "system should start on line 1");
    require(tokens[0].range.begin.column == 1, "system should start at column 1");
    require(tokens[1].range.begin.line == 2, "identifier should start on line 2");
    require(tokens[1].range.begin.column == 3, "identifier should start at column 3");
}

void lexer_reports_unterminated_string()
{
    statespec::DiagnosticBag diagnostics;
    auto tokens = lex_text("\"unterminated", diagnostics);
    require(diagnostics.has_errors(), "unterminated string should produce diagnostic");
    require(
        tokens[0].kind == statespec::TokenKind::StringLiteral,
        "unterminated string should still produce a token for recovery"
    );
}

void lexer_reports_unterminated_block_comment()
{
    statespec::DiagnosticBag diagnostics;
    auto tokens = lex_text("system A { /* missing end", diagnostics);
    require(diagnostics.has_errors(), "unterminated block comment should produce diagnostic");
    require(
        tokens[0].kind == statespec::TokenKind::KeywordSystem,
        "tokens before unterminated comment should be preserved"
    );
    require(
        tokens.back().kind == statespec::TokenKind::EndOfFile,
        "lexer should still emit EOF after unterminated comment"
    );
}

void parser_parses_empty_system()
{
    const auto spec = parse_text("statespec 0.1; system OrderSystem {}");
    require(spec.version == "0.1", "parser should capture version");
    require(spec.system.has_value(), "parser should capture system");
    require(spec.system->name == "OrderSystem", "parser should capture system name");
}

void parser_parses_entity_fields_and_state_machine()
{
    const auto spec = parse_text(R"sspec(
        system OrderSystem {
          entity Order {
            key tenant_id, order_id
            fields {
              tenant_id string
              order_id string
              status string
            }
            state_machine {
              state Creating
              state Active
              state Failed { terminal: true }
              initial Creating
              terminal [Failed]
              Creating -> Active
              Creating -> Failed
            }
          }
        }
    )sspec");

    require(spec.system.has_value(), "parser should parse system");
    require(spec.system->entities.size() == 1, "parser should parse one entity");
    const auto& entity = spec.system->entities[0];
    require(entity.name == "Order", "parser should parse entity name");
    require(entity.key_fields.size() == 2, "parser should parse composite key fields");
    require(entity.fields.size() == 3, "parser should parse fields");
    require(entity.fields[0].name == "tenant_id", "parser should parse field name");
    require(entity.fields[0].type == "string", "parser should parse field type");
    require(entity.state_machine.has_value(), "parser should parse state machine");
    require(entity.state_machine->states.size() == 3, "parser should parse states");
    require(entity.state_machine->initial_state == "Creating", "parser should parse initial state");
    require(entity.state_machine->terminal_states.size() == 1, "parser should parse terminal states");
    require(entity.state_machine->transitions.size() == 2, "parser should parse transitions");
}

void parser_parses_workflow_steps()
{
    const auto spec = parse_text(R"sspec(
        system OrderSystem {
          workflow OrderProcessing {
            version 1
            singleton false
            expected_execution_time PT5M
            start validate_order
            step validate_order {
              expected_execution_time PT10S
              max_retries 2
            }
            step charge_payment {
              expected_execution_time PT30S
              max_retries 3
            }
          }
        }
    )sspec");

    require(spec.system.has_value(), "parser should parse system");
    require(spec.system->workflows.size() == 1, "parser should parse one workflow");
    const auto& workflow = spec.system->workflows[0];
    require(workflow.name == "OrderProcessing", "parser should parse workflow name");
    require(workflow.version == 1, "parser should parse workflow version");
    require(workflow.singleton == false, "parser should parse singleton value");
    require(workflow.expected_execution_time == "PT5M", "parser should parse workflow duration");
    require(workflow.start_step == "validate_order", "parser should parse start step");
    require(workflow.steps.size() == 2, "parser should parse workflow steps");
    require(workflow.steps[0].name == "validate_order", "parser should parse first step name");
    require(workflow.steps[0].expected_execution_time == "PT10S", "parser should parse step duration");
    require(workflow.steps[0].max_retries == 2, "parser should parse max_retries");
}

void parser_reports_missing_system()
{
    statespec::DiagnosticBag diagnostics;
    auto spec = parse_text("entity Order {}", diagnostics);
    require(!spec.system.has_value(), "parser should not synthesize missing system");
    require(diagnostics.has_errors(), "parser should report missing system");
}

void validator_rejects_missing_system()
{
    statespec::Spec spec;
    statespec::DiagnosticBag diagnostics;
    statespec::Validator validator;
    validator.validate(spec, diagnostics);
    require(diagnostics.has_errors(), "validator should reject spec without system");
}

} // namespace

int main()
{
    diagnostic_bag_records_errors();
    symbol_table_rejects_duplicates();
    lexer_emits_eof_for_empty_source();
    lexer_tokenizes_system_declaration();
    lexer_skips_comments();
    lexer_tokenizes_literals();
    lexer_tokenizes_operators_and_punctuation();
    lexer_tracks_line_and_column();
    lexer_reports_unterminated_string();
    lexer_reports_unterminated_block_comment();
    parser_parses_empty_system();
    parser_parses_entity_fields_and_state_machine();
    parser_parses_workflow_steps();
    parser_reports_missing_system();
    validator_rejects_missing_system();

    std::cout << "smoke tests passed\n";
    return 0;
}
