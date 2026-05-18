#include "statespec/formatter.hpp"

#include <algorithm>
#include <sstream>

namespace statespec
{

namespace
{

bool is_word_token(TokenKind kind)
{
    switch (kind)
    {
    case TokenKind::Identifier:
    case TokenKind::StringLiteral:
    case TokenKind::IntegerLiteral:
    case TokenKind::DecimalLiteral:
    case TokenKind::BooleanLiteral:
    case TokenKind::DurationLiteral:
    case TokenKind::KeywordStatespec:
    case TokenKind::KeywordInclude:
    case TokenKind::KeywordImport:
    case TokenKind::KeywordAs:
    case TokenKind::KeywordSystem:
    case TokenKind::KeywordNamespace:
    case TokenKind::KeywordValue:
    case TokenKind::KeywordEnum:
    case TokenKind::KeywordShape:
    case TokenKind::KeywordExternalSystem:
    case TokenKind::KeywordFeatureFlag:
    case TokenKind::KeywordLog:
    case TokenKind::KeywordMetric:
    case TokenKind::KeywordEntity:
    case TokenKind::KeywordKey:
    case TokenKind::KeywordVersion:
    case TokenKind::KeywordFields:
    case TokenKind::KeywordStateMachine:
    case TokenKind::KeywordState:
    case TokenKind::KeywordInitial:
    case TokenKind::KeywordTerminal:
    case TokenKind::KeywordOwnership:
    case TokenKind::KeywordAuthority:
    case TokenKind::KeywordSystemOfRecord:
    case TokenKind::KeywordLifecycle:
    case TokenKind::KeywordControl:
    case TokenKind::KeywordRelations:
    case TokenKind::KeywordParent:
    case TokenKind::KeywordRef:
    case TokenKind::KeywordOptional:
    case TokenKind::KeywordChildren:
    case TokenKind::KeywordInvariants:
    case TokenKind::KeywordIndexes:
    case TokenKind::KeywordAnnotations:
    case TokenKind::KeywordEvent:
    case TokenKind::KeywordQueue:
    case TokenKind::KeywordMessage:
    case TokenKind::KeywordPayload:
    case TokenKind::KeywordLease:
    case TokenKind::KeywordWorker:
    case TokenKind::KeywordApiServer:
    case TokenKind::KeywordApi:
    case TokenKind::KeywordBehavior:
    case TokenKind::KeywordWorkflow:
    case TokenKind::KeywordStep:
    case TokenKind::KeywordChildSet:
    case TokenKind::KeywordPolicy:
    case TokenKind::KeywordWhen:
    case TokenKind::KeywordWhere:
    case TokenKind::KeywordRequire:
    case TokenKind::KeywordSet:
    case TokenKind::KeywordEmit:
    case TokenKind::KeywordEmits:
    case TokenKind::KeywordEnqueue:
    case TokenKind::KeywordAcquire:
    case TokenKind::KeywordRenew:
    case TokenKind::KeywordRelease:
    case TokenKind::KeywordStart:
    case TokenKind::KeywordLoad:
    case TokenKind::KeywordAllocates:
    case TokenKind::KeywordReturns:
    case TokenKind::KeywordAtomic:
    case TokenKind::KeywordForEach:
    case TokenKind::KeywordIn:
    case TokenKind::KeywordCreate:
    case TokenKind::KeywordChild:
    case TokenKind::KeywordObserve:
    case TokenKind::KeywordMove:
    case TokenKind::KeywordFrom:
    case TokenKind::KeywordTo:
    case TokenKind::KeywordTransitionTo:
    case TokenKind::KeywordComplete:
    case TokenKind::KeywordFail:
    case TokenKind::KeywordReserve:
    case TokenKind::KeywordMaterialize:
    case TokenKind::KeywordReconcile:
    case TokenKind::KeywordAllow:
    case TokenKind::KeywordDeny:
    case TokenKind::KeywordQuota:
    case TokenKind::KeywordAudit:
    case TokenKind::KeywordTenant:
    case TokenKind::KeywordScopedBy:
    case TokenKind::KeywordMethod:
    case TokenKind::KeywordPath:
    case TokenKind::KeywordInput:
    case TokenKind::KeywordOutput:
    case TokenKind::KeywordError:
    case TokenKind::KeywordAuthz:
    case TokenKind::KeywordStarts:
    case TokenKind::KeywordEnqueues:
    case TokenKind::KeywordServes:
    case TokenKind::KeywordPolls:
    case TokenKind::KeywordExecutes:
    case TokenKind::KeywordSingleton:
    case TokenKind::KeywordConcurrency:
        return true;
    default:
        return false;
    }
}

bool is_binary_operator(TokenKind kind)
{
    switch (kind)
    {
    case TokenKind::Arrow:
    case TokenKind::Equals:
    case TokenKind::EqualEqual:
    case TokenKind::BangEqual:
    case TokenKind::LessEqual:
    case TokenKind::GreaterEqual:
    case TokenKind::Plus:
    case TokenKind::Minus:
    case TokenKind::Star:
    case TokenKind::Slash:
    case TokenKind::Percent:
    case TokenKind::AndAnd:
    case TokenKind::OrOr:
        return true;
    default:
        return false;
    }
}

bool starts_compact(TokenKind kind)
{
    return kind == TokenKind::LeftParen || kind == TokenKind::LeftBracket ||
           kind == TokenKind::Less || kind == TokenKind::Dot || kind == TokenKind::Comma ||
           kind == TokenKind::Semicolon || kind == TokenKind::Colon ||
           kind == TokenKind::Question || kind == TokenKind::RightParen ||
           kind == TokenKind::RightBracket || kind == TokenKind::Greater;
}

bool ends_compact(TokenKind kind)
{
    return kind == TokenKind::LeftParen || kind == TokenKind::LeftBracket ||
           kind == TokenKind::Less || kind == TokenKind::Dot || kind == TokenKind::Bang;
}

std::string token_text(const Token& token)
{
    return token.lexeme;
}

void write_indent(
    std::ostringstream& out,
    int indent
)
{
    for (int i = 0; i < indent; ++i)
    {
        out << "  ";
    }
}

bool is_feature_flag_member_start(const Token& token)
{
    return token.kind == TokenKind::Identifier &&
           (token.lexeme == "type" || token.lexeme == "default" || token.lexeme == "scope" ||
            token.lexeme == "owner" || token.lexeme == "description" || token.lexeme == "expires");
}

bool is_state_option_member_start(const Token& token)
{
    return token.kind == TokenKind::KeywordTerminal ||
           (token.kind == TokenKind::Identifier && token.lexeme == "garbage_collection");
}

bool is_garbage_collection_member_start(const Token& token)
{
    return token.kind == TokenKind::Identifier &&
           (token.lexeme == "after" || token.lexeme == "mode");
}

} // namespace

std::string format_tokens(
    const std::vector<Token>& tokens,
    DiagnosticBag& diagnostics
)
{
    if (diagnostics.has_errors())
    {
        return "";
    }

    std::ostringstream out;
    int indent = 0;
    bool at_line_start = true;
    bool previous_was_blank_line = false;
    TokenKind previous_kind = TokenKind::EndOfFile;
    std::size_t previous_source_line = 0;
    std::vector<TokenKind> block_stack;
    std::vector<std::string> block_name_stack;

    auto newline = [&](bool blank_after = false)
    {
        out << '\n';
        at_line_start = true;
        previous_kind = TokenKind::EndOfFile;
        if (blank_after && !previous_was_blank_line)
        {
            out << '\n';
            previous_was_blank_line = true;
        }
        else
        {
            previous_was_blank_line = false;
        }
    };

    for (std::size_t i = 0; i < tokens.size(); ++i)
    {
        const auto& token = tokens[i];
        if (token.kind == TokenKind::EndOfFile)
        {
            break;
        }

        const bool in_feature_flag_block =
            !block_stack.empty() && block_stack.back() == TokenKind::KeywordFeatureFlag;
        const bool in_state_options_block =
            !block_stack.empty() && block_stack.back() == TokenKind::KeywordState;
        const bool in_garbage_collection_block =
            !block_name_stack.empty() && block_name_stack.back() == "garbage_collection";
        if (!at_line_start &&
            (token.range.begin.line > previous_source_line ||
             (in_feature_flag_block && is_feature_flag_member_start(token) &&
              previous_kind != TokenKind::LeftBrace) ||
             (in_state_options_block && is_state_option_member_start(token) &&
              previous_kind != TokenKind::LeftBrace) ||
             (in_garbage_collection_block && is_garbage_collection_member_start(token) &&
              previous_kind != TokenKind::LeftBrace)))
        {
            newline(token.range.begin.line > previous_source_line + 1);
        }

        if (token.kind == TokenKind::RightBrace)
        {
            if (!at_line_start)
            {
                newline();
            }
            indent = std::max(0, indent - 1);
            if (!block_stack.empty())
            {
                block_stack.pop_back();
            }
            if (!block_name_stack.empty())
            {
                block_name_stack.pop_back();
            }
            write_indent(out, indent);
            out << '}';
            at_line_start = false;
            previous_kind = token.kind;
            const bool top_level_close = indent == 0;
            if (tokens[i + 1].kind != TokenKind::EndOfFile)
            {
                newline(
                    top_level_close || token.range.end.line + 1 < tokens[i + 1].range.begin.line
                );
            }
            previous_source_line = token.range.begin.line;
            continue;
        }

        if (at_line_start)
        {
            write_indent(out, indent);
            at_line_start = false;
        }
        else if (is_binary_operator(token.kind) || is_binary_operator(previous_kind))
        {
            out << ' ';
        }
        else if (token.kind == TokenKind::LeftBrace)
        {
            out << ' ';
        }
        else if (token.kind == TokenKind::Comma)
        {
        }
        else if (previous_kind == TokenKind::Comma)
        {
            out << ' ';
        }
        else if (is_word_token(token.kind) && is_word_token(previous_kind))
        {
            out << ' ';
        }
        else if (!starts_compact(token.kind) && !ends_compact(previous_kind))
        {
            out << ' ';
        }

        out << token_text(token);

        if (token.kind == TokenKind::LeftBrace)
        {
            TokenKind block_kind = previous_kind;
            std::string block_name;
            if (previous_kind == TokenKind::Identifier && i >= 2)
            {
                block_kind = tokens[i - 2].kind;
                block_name = tokens[i - 1].lexeme;
            }
            block_stack.push_back(block_kind);
            block_name_stack.push_back(block_name);
            ++indent;
            newline();
        }
        else if (token.kind == TokenKind::Semicolon)
        {
            newline();
        }

        previous_kind = token.kind;
        previous_source_line = token.range.begin.line;
    }

    auto result = out.str();
    while (!result.empty() && result.back() == '\n')
    {
        result.pop_back();
    }
    result.push_back('\n');
    return result;
}

} // namespace statespec
