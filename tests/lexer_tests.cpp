#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/token.hpp"

#include <cstddef>
#include <vector>

namespace
{

void lexer_emits_eof_for_empty_source()
{
    const auto tokens = statespec::test::lex_text("");
    statespec::test::require(tokens.size() == 1, "empty source should produce one EOF token");
    statespec::test::require(
        tokens[0].kind == statespec::TokenKind::EndOfFile, "empty source token should be EOF"
    );
}

void lexer_tokenizes_system_declaration()
{
    const auto tokens = statespec::test::lex_text(
        "statespec 0.1; include \"./workflow-launch-control.sspec\"; system OrderSystem {}"
    );
    statespec::test::require(
        tokens.size() == 11, "system declaration should produce expected token count including EOF"
    );
    statespec::test::require(
        tokens[0].kind == statespec::TokenKind::KeywordStatespec, "statespec should be a keyword"
    );
    statespec::test::require(
        tokens[1].kind == statespec::TokenKind::DecimalLiteral, "0.1 should be a decimal literal"
    );
    statespec::test::require(
        tokens[2].kind == statespec::TokenKind::Semicolon, "semicolon should be tokenized"
    );
    statespec::test::require(
        tokens[3].kind == statespec::TokenKind::KeywordInclude, "include should be a keyword"
    );
    statespec::test::require(
        tokens[4].kind == statespec::TokenKind::StringLiteral,
        "include path should be string literal"
    );
    statespec::test::require(
        tokens[6].kind == statespec::TokenKind::KeywordSystem, "system should be a keyword"
    );
    statespec::test::require(
        tokens[7].kind == statespec::TokenKind::Identifier, "system name should be identifier"
    );
    statespec::test::require(
        tokens[7].lexeme == "OrderSystem", "identifier lexeme should be preserved"
    );
    statespec::test::require(
        tokens[8].kind == statespec::TokenKind::LeftBrace, "left brace should be tokenized"
    );
    statespec::test::require(
        tokens[9].kind == statespec::TokenKind::RightBrace, "right brace should be tokenized"
    );
    statespec::test::require(
        tokens[10].kind == statespec::TokenKind::EndOfFile, "last token should be EOF"
    );
}

void lexer_skips_comments()
{
    const auto tokens =
        statespec::test::lex_text("// line comment\nsystem A { /* block comment */ entity B {} }");
    statespec::test::require(
        tokens[0].kind == statespec::TokenKind::KeywordSystem, "line comment should be skipped"
    );
    statespec::test::require(
        tokens[1].lexeme == "A", "identifier after comment should be tokenized"
    );
    statespec::test::require(
        tokens[3].kind == statespec::TokenKind::KeywordEntity, "block comment should be skipped"
    );
}

void lexer_tokenizes_observability_keywords()
{
    const auto tokens =
        statespec::test::lex_text("log WorkflowDecision {} metric WorkflowAttempts {}");
    statespec::test::require(
        tokens[0].kind == statespec::TokenKind::KeywordLog, "log should be a keyword"
    );
    statespec::test::require(
        tokens[1].kind == statespec::TokenKind::Identifier, "log name should be identifier"
    );
    statespec::test::require(
        tokens[4].kind == statespec::TokenKind::KeywordMetric, "metric should be a keyword"
    );
    statespec::test::require(
        tokens[5].kind == statespec::TokenKind::Identifier, "metric name should be identifier"
    );
}

void lexer_tokenizes_literals()
{
    const auto tokens =
        statespec::test::lex_text("true false 42 7.5 \"hello\\nworld\" PT30S P1DT2H3M4S");
    statespec::test::require(
        tokens[0].kind == statespec::TokenKind::BooleanLiteral, "true should be boolean literal"
    );
    statespec::test::require(
        tokens[1].kind == statespec::TokenKind::BooleanLiteral, "false should be boolean literal"
    );
    statespec::test::require(
        tokens[2].kind == statespec::TokenKind::IntegerLiteral, "42 should be integer literal"
    );
    statespec::test::require(
        tokens[3].kind == statespec::TokenKind::DecimalLiteral, "7.5 should be decimal literal"
    );
    statespec::test::require(
        tokens[4].kind == statespec::TokenKind::StringLiteral,
        "quoted string should be string literal"
    );
    statespec::test::require(
        tokens[5].kind == statespec::TokenKind::DurationLiteral, "PT30S should be duration literal"
    );
    statespec::test::require(
        tokens[6].kind == statespec::TokenKind::DurationLiteral,
        "P1DT2H3M4S should be duration literal"
    );
}

void lexer_tokenizes_operators_and_punctuation()
{
    const auto tokens =
        statespec::test::lex_text("{}()[]<>,:;.? -> = == != <= >= + - * / % ! && ||");
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

    statespec::test::require(
        tokens.size() == expected.size(), "operator token count should match expected count"
    );
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        statespec::test::require(
            tokens[i].kind == expected[i], "operator token kind should match expected sequence"
        );
    }
}

void lexer_tracks_line_and_column()
{
    const auto tokens = statespec::test::lex_text("system\n  OrderSystem");
    statespec::test::require(tokens[0].range.begin.line == 1, "system should start on line 1");
    statespec::test::require(tokens[0].range.begin.column == 1, "system should start at column 1");
    statespec::test::require(tokens[1].range.begin.line == 2, "identifier should start on line 2");
    statespec::test::require(
        tokens[1].range.begin.column == 3, "identifier should start at column 3"
    );
}

void lexer_reports_unterminated_string()
{
    statespec::DiagnosticBag diagnostics;
    auto tokens = statespec::test::lex_text("\"unterminated", diagnostics);
    statespec::test::require(
        diagnostics.has_errors(), "unterminated string should produce diagnostic"
    );
    statespec::test::require(
        tokens[0].kind == statespec::TokenKind::StringLiteral,
        "unterminated string should still produce a token for recovery"
    );
}

void lexer_reports_unterminated_block_comment()
{
    statespec::DiagnosticBag diagnostics;
    auto tokens = statespec::test::lex_text("system A { /* missing end", diagnostics);
    statespec::test::require(
        diagnostics.has_errors(), "unterminated block comment should produce diagnostic"
    );
    statespec::test::require(
        tokens[0].kind == statespec::TokenKind::KeywordSystem,
        "tokens before unterminated comment should be preserved"
    );
    statespec::test::require(
        tokens.back().kind == statespec::TokenKind::EndOfFile,
        "lexer should still emit EOF after unterminated comment"
    );
}

} // namespace

TEST_CASE("lexer emits EOF for empty source")
{
    lexer_emits_eof_for_empty_source();
}

TEST_CASE("lexer tokenizes system declarations")
{
    lexer_tokenizes_system_declaration();
}

TEST_CASE("lexer skips comments")
{
    lexer_skips_comments();
}

TEST_CASE("lexer tokenizes observability keywords")
{
    lexer_tokenizes_observability_keywords();
}

TEST_CASE("lexer tokenizes literals")
{
    lexer_tokenizes_literals();
}

TEST_CASE("lexer tokenizes operators and punctuation")
{
    lexer_tokenizes_operators_and_punctuation();
}

TEST_CASE("lexer tracks line and column")
{
    lexer_tracks_line_and_column();
}

TEST_CASE("lexer reports unterminated strings")
{
    lexer_reports_unterminated_string();
}

TEST_CASE("lexer reports unterminated block comments")
{
    lexer_reports_unterminated_block_comment();
}
