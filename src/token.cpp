#include "statespec/token.hpp"

namespace statespec {

std::string token_kind_name(TokenKind kind)
{
    switch (kind)
    {
        case TokenKind::EndOfFile: return "EndOfFile";
        case TokenKind::Identifier: return "Identifier";
        case TokenKind::StringLiteral: return "StringLiteral";
        case TokenKind::IntegerLiteral: return "IntegerLiteral";
        case TokenKind::DecimalLiteral: return "DecimalLiteral";
        case TokenKind::BooleanLiteral: return "BooleanLiteral";
        case TokenKind::DurationLiteral: return "DurationLiteral";
        case TokenKind::LeftBrace: return "LeftBrace";
        case TokenKind::RightBrace: return "RightBrace";
        case TokenKind::LeftParen: return "LeftParen";
        case TokenKind::RightParen: return "RightParen";
        case TokenKind::LeftBracket: return "LeftBracket";
        case TokenKind::RightBracket: return "RightBracket";
        case TokenKind::Less: return "Less";
        case TokenKind::Greater: return "Greater";
        case TokenKind::Comma: return "Comma";
        case TokenKind::Colon: return "Colon";
        case TokenKind::Semicolon: return "Semicolon";
        case TokenKind::Dot: return "Dot";
        case TokenKind::Question: return "Question";
        case TokenKind::Arrow: return "Arrow";
        case TokenKind::Equals: return "Equals";
        case TokenKind::EqualEqual: return "EqualEqual";
        case TokenKind::BangEqual: return "BangEqual";
        case TokenKind::LessEqual: return "LessEqual";
        case TokenKind::GreaterEqual: return "GreaterEqual";
        case TokenKind::Plus: return "Plus";
        case TokenKind::Minus: return "Minus";
        case TokenKind::Star: return "Star";
        case TokenKind::Slash: return "Slash";
        case TokenKind::Percent: return "Percent";
        case TokenKind::Bang: return "Bang";
        case TokenKind::AndAnd: return "AndAnd";
        case TokenKind::OrOr: return "OrOr";
        case TokenKind::KeywordSystem: return "KeywordSystem";
        case TokenKind::KeywordEntity: return "KeywordEntity";
        case TokenKind::KeywordStateMachine: return "KeywordStateMachine";
        case TokenKind::KeywordWorkflow: return "KeywordWorkflow";
        case TokenKind::KeywordQueue: return "KeywordQueue";
        case TokenKind::KeywordLease: return "KeywordLease";
        case TokenKind::KeywordWorker: return "KeywordWorker";
        case TokenKind::KeywordPolicy: return "KeywordPolicy";
        case TokenKind::KeywordApi: return "KeywordApi";
        case TokenKind::KeywordGenerate: return "KeywordGenerate";
        case TokenKind::Unknown: return "Unknown";
    }
    return "Unknown";
}

} // namespace statespec
