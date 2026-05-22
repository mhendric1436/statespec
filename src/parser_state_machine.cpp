#include "statespec/parser.hpp"

#include "parser_helpers.hpp"
#include "statespec/language_constants.hpp"

#include <utility>

namespace statespec
{

using parser_detail::is_named_identifier;

StateMachineDecl Parser::parse_state_machine_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordStateMachine, "expected state_machine block", diagnostics);
    StateMachineDecl state_machine;
    consume(TokenKind::LeftBrace, "expected '{' after state_machine", diagnostics);

    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordState))
        {
            state_machine.states.push_back(parse_state_decl(diagnostics));
        }
        else if (match(TokenKind::KeywordInitial))
        {
            state_machine.initial_state =
                consume(TokenKind::Identifier, "expected initial state", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordTerminal))
        {
            if (check(TokenKind::LeftBracket))
            {
                state_machine.terminal_states = parse_identifier_list(diagnostics);
            }
            else
            {
                state_machine.terminal_states.push_back(
                    consume(TokenKind::Identifier, "expected terminal state", diagnostics).lexeme
                );
            }
            consume_optional_semicolon();
        }
        else if (check(TokenKind::Identifier) && peek(1).kind == TokenKind::Arrow)
        {
            const auto from = advance();
            advance();
            const auto to =
                consume(TokenKind::Identifier, "expected transition target state", diagnostics);
            TransitionDecl transition;
            transition.from = from.lexeme;
            transition.to = to.lexeme;
            transition.range = SourceRange{from.range.begin, to.range.end};
            if (check(TokenKind::LeftBrace))
            {
                skip_balanced_block();
            }
            consume_optional_semicolon();
            state_machine.transitions.push_back(transition);
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }

    consume(TokenKind::RightBrace, "expected '}' after state_machine block", diagnostics);
    state_machine.range = SourceRange{start.range.begin, previous().range.end};
    return state_machine;
}

StateDecl Parser::parse_state_decl(DiagnosticBag& diagnostics)
{
    const auto name = consume(TokenKind::Identifier, "expected state name", diagnostics);
    StateDecl state;
    state.name = name.lexeme;
    state.range = name.range;

    if (match(TokenKind::LeftBrace))
    {
        while (!check(TokenKind::RightBrace) && !is_at_end())
        {
            if (match(TokenKind::KeywordTerminal))
            {
                consume(TokenKind::Colon, "expected ':' after terminal", diagnostics);
                const auto terminal =
                    consume(TokenKind::BooleanLiteral, "expected terminal boolean", diagnostics);
                state.terminal = terminal.lexeme == SyntaxBooleanTrue;
                consume_optional_semicolon();
            }
            else if (is_named_identifier(peek(), SyntaxIdentifierGarbageCollection))
            {
                auto policy = parse_garbage_collection_policy_decl(diagnostics);
                if (state.garbage_collection.has_value())
                {
                    state.duplicate_garbage_collection_range = policy.range;
                }
                else
                {
                    state.garbage_collection = std::move(policy);
                }
            }
            else
            {
                skip_unknown_declaration(diagnostics);
            }
        }
        consume(TokenKind::RightBrace, "expected '}' after state options", diagnostics);
        state.range = SourceRange{name.range.begin, previous().range.end};
    }

    consume_optional_semicolon();
    return state;
}

GarbageCollectionPolicyDecl Parser::parse_garbage_collection_policy_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::Identifier, "expected garbage_collection block", diagnostics);
    GarbageCollectionPolicyDecl policy;
    consume(TokenKind::LeftBrace, "expected '{' after garbage_collection", diagnostics);

    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (is_named_identifier(peek(), SyntaxIdentifierAfter))
        {
            advance();
            consume(TokenKind::Colon, "expected ':' after garbage_collection.after", diagnostics);
            policy.after = parse_simple_value(diagnostics, "garbage_collection.after");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), SyntaxIdentifierMode))
        {
            advance();
            consume(TokenKind::Colon, "expected ':' after garbage_collection.mode", diagnostics);
            policy.mode = parse_simple_value(diagnostics, "garbage_collection.mode");
            consume_optional_semicolon();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }

    consume(TokenKind::RightBrace, "expected '}' after garbage_collection block", diagnostics);
    policy.range = SourceRange{start.range.begin, previous().range.end};
    return policy;
}

} // namespace statespec
