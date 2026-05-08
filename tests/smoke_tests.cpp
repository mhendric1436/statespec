#include "statespec/diagnostic.hpp"
#include "statespec/lexer.hpp"
#include "statespec/source.hpp"
#include "statespec/symbol_table.hpp"
#include "statespec/token.hpp"
#include "statespec/validator.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "test failed: " << message << '\n';
        std::exit(1);
    }
}

std::vector<statespec::Token> lex_text(const std::string& text, statespec::DiagnosticBag& diagnostics)
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
    require(table.insert(statespec::Symbol{statespec::SymbolKind::Entity, "Order", {}}), "first symbol insert should succeed");
    require(!table.insert(statespec::Symbol{statespec::SymbolKind::Entity, "Order", {}}), "duplicate symbol insert should fail");
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
    require(tokens.size() == 8, "system declaration should produce expected token count including EOF");
    require(tokens[0].kind == statespec::TokenKind::KeywordStatespec, "statespec should be a keyword");
    require(tokens[1].kind == statespec::TokenKind::DecimalLiteral, "0.1 should be a decimal literal");
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
    require(tokens[0].kind == statespec::TokenKind::KeywordSystem, "line comment should be skipped");
    require(tokens[1].lexeme == "A", "identifier after comment should be tokenized");
    require(tokens[3].kind == statespec::TokenKind::KeywordEntity, "block comment should be skipped");
}

void lexer_tokenizes_literals()
{
    const auto tokens = lex_text("true false 42 7.5 \"hello\\nworld\" PT30S P1DT2H3M4S");
    require(tokens[0].kind == statespec::TokenKind::BooleanLiteral, "true should be boolean literal");
    require(tokens[1].kind == statespec::TokenKind::BooleanLiteral, "false should be boolean literal");
    require(tokens[2].kind == statespec::TokenKind::IntegerLiteral, "42 should be integer literal");
    require(tokens[3].kind == statespec::TokenKind::DecimalLiteral, "7.5 should be decimal literal");
    require(tokens[4].kind == statespec::TokenKind::StringLiteral, "quoted string should be string literal");
    require(tokens[5].kind == statespec::TokenKind::DurationLiteral, "PT30S should be duration literal");
    require(tokens[6].kind == statespec::TokenKind::DurationLiteral, "P1DT2H3M4S should be duration literal");
}

void lexer_tokenizes_operators_and_punctuation()
{
    const auto tokens = lex_text("{}()[]<>,:;.? -> = == != <= >= + - * / % ! && ||");
    const std::vector<statespec::TokenKind> expected{
        statespec::TokenKind::LeftBrace,
        statespec::TokenKind::RightBrace,
        statespec::TokenKind::LeftParen,
        statespec::TokenKind::RightParen,
        statespec::TokenKind::LeftBracket,
        statespec::TokenKind::RightBracket,
        statespec::TokenKind::Less,
        statespec::TokenKind::Greater,
        statespec::TokenKind::Comma,
        statespec::TokenKind::Colon,
        statespec::TokenKind::Semicolon,
        statespec::TokenKind::Dot,
        statespec::TokenKind::Question,
        statespec::TokenKind::Arrow,
        statespec::TokenKind::Equals,
        statespec::TokenKind::EqualEqual,
        statespec::TokenKind::BangEqual,
        statespec::TokenKind::LessEqual,
        statespec::TokenKind::GreaterEqual,
        statespec::TokenKind::Plus,
        statespec::TokenKind::Minus,
        statespec::TokenKind::Star,
        statespec::TokenKind::Slash,
        statespec::TokenKind::Percent,
        statespec::TokenKind::Bang,
        statespec::TokenKind::AndAnd,
        statespec::TokenKind::OrOr,
        statespec::TokenKind::EndOfFile,
    };

    require(tokens.size() == expected.size(), "operator token count should match expected count");
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        require(tokens[i].kind == expected[i], "operator token kind should match expected sequence");
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
    require(tokens[0].kind == statespec::TokenKind::StringLiteral, "unterminated string should still produce a token for recovery");
}

void lexer_reports_unterminated_block_comment()
{
    statespec::DiagnosticBag diagnostics;
    auto tokens = lex_text("system A { /* missing end", diagnostics);
    require(diagnostics.has_errors(), "unterminated block comment should produce diagnostic");
    require(tokens[0].kind == statespec::TokenKind::KeywordSystem, "tokens before unterminated comment should be preserved");
    require(tokens.back().kind == statespec::TokenKind::EndOfFile, "lexer should still emit EOF after unterminated comment");
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
    validator_rejects_missing_system();

    std::cout << "smoke tests passed\n";
    return 0;
}
