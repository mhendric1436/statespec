#pragma once

#include "statespec/source.hpp"

#include <string>

namespace statespec {

enum class TokenKind {
    EndOfFile,
    Identifier,
    StringLiteral,
    IntegerLiteral,
    DecimalLiteral,
    BooleanLiteral,
    DurationLiteral,
    LeftBrace,
    RightBrace,
    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    Less,
    Greater,
    Comma,
    Colon,
    Semicolon,
    Dot,
    Question,
    Arrow,
    Equals,
    EqualEqual,
    BangEqual,
    LessEqual,
    GreaterEqual,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Bang,
    AndAnd,
    OrOr,
    KeywordSystem,
    KeywordEntity,
    KeywordStateMachine,
    KeywordWorkflow,
    KeywordQueue,
    KeywordLease,
    KeywordWorker,
    KeywordPolicy,
    KeywordApi,
    KeywordGenerate,
    Unknown,
};

struct Token {
    TokenKind kind = TokenKind::Unknown;
    std::string lexeme;
    SourceRange range;
};

std::string token_kind_name(TokenKind kind);

} // namespace statespec
