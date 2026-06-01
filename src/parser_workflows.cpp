#include "statespec/parser.hpp"

#include "parser_helpers.hpp"
#include "statespec/language_constants.hpp"

#include <utility>
#include <vector>

namespace statespec
{

using parser_detail::is_named_identifier;
using parser_detail::parse_int_or_zero;

WorkflowDecl Parser::parse_workflow_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordWorkflow, "expected workflow declaration", diagnostics);
    auto name = consume(TokenKind::Identifier, "expected workflow name", diagnostics);
    WorkflowDecl workflow;
    workflow.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after workflow name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordVersion))
        {
            workflow.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordVersion}, previous().range}
            );
            workflow.version = parse_int_or_zero(
                consume(TokenKind::IntegerLiteral, "expected workflow version integer", diagnostics)
            );
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordSingleton))
        {
            workflow.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordSingleton}, previous().range}
            );
            const auto value =
                consume(TokenKind::BooleanLiteral, "expected singleton boolean", diagnostics);
            workflow.singleton = value.lexeme == SyntaxBooleanTrue;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), SyntaxIdentifierExpectedExecutionTime))
        {
            const auto expected_time_start = advance();
            workflow.member_order.push_back(
                BlockMemberOrder{
                    std::string{SyntaxIdentifierExpectedExecutionTime}, expected_time_start.range
                }
            );
            auto value =
                consume(TokenKind::DurationLiteral, "expected workflow duration", diagnostics);
            workflow.expected_execution_time = value.lexeme;
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordStart))
        {
            workflow.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordStart}, previous().range}
            );
            workflow.start_step =
                consume(TokenKind::Identifier, "expected start step name", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordOn))
        {
            workflow.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordOn}, previous().range}
            );
            workflow.on = parse_qualified_name(diagnostics, "workflow trigger");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordInput))
        {
            workflow.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordInput}, previous().range}
            );
            workflow.input = parse_qualified_name(diagnostics, "workflow input");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordState))
        {
            workflow.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordState}, previous().range}
            );
            workflow.state = parse_qualified_name(diagnostics, "workflow state");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordLoad))
        {
            auto load = parse_workflow_load_decl(diagnostics);
            workflow.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordLoad}, load.range}
            );
            workflow.loads.push_back(std::move(load));
        }
        else if (check(TokenKind::KeywordChildSet))
        {
            auto child_set = parse_workflow_child_set_decl(diagnostics);
            workflow.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordChildSet}, child_set.range}
            );
            workflow.child_sets.push_back(std::move(child_set));
        }
        else if (check(TokenKind::KeywordStep))
        {
            auto step = parse_workflow_step_decl(diagnostics);
            workflow.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordStep}, step.range}
            );
            workflow.steps.push_back(std::move(step));
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after workflow block", diagnostics);

    workflow.range = SourceRange{start.range.begin, previous().range.end};
    return workflow;
}

WorkflowLoadDecl Parser::parse_workflow_load_decl(DiagnosticBag& diagnostics)
{
    const auto start = previous();
    WorkflowLoadDecl load;
    load.entity = parse_qualified_name(diagnostics, "workflow load entity");
    if (!is_named_identifier(peek(), SyntaxIdentifierBy))
    {
        diagnostics.error(peek().range, "SSPEC0200", "expected by in workflow load");
    }
    else
    {
        advance();
    }
    load.key_field =
        consume(TokenKind::Identifier, "expected workflow load key", diagnostics).lexeme;
    consume(TokenKind::KeywordAs, "expected as in workflow load", diagnostics);
    load.binding =
        consume(TokenKind::Identifier, "expected workflow load binding", diagnostics).lexeme;
    consume_optional_semicolon();
    load.range = SourceRange{start.range.begin, previous().range.end};
    return load;
}

WorkflowChildSetDecl Parser::parse_workflow_child_set_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordChildSet, "expected child_set", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected child_set name", diagnostics);
    WorkflowChildSetDecl child_set;
    child_set.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after child_set name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordEntity))
        {
            child_set.entity = parse_qualified_name(diagnostics, "child_set entity");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "parent_field"))
        {
            advance();
            child_set.parent_field =
                consume(TokenKind::Identifier, "expected child_set parent field", diagnostics)
                    .lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "id_type"))
        {
            advance();
            child_set.id_type = parse_type_name(diagnostics);
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "pending"))
        {
            advance();
            child_set.pending =
                consume(TokenKind::Identifier, "expected child_set pending field", diagnostics)
                    .lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "creating"))
        {
            advance();
            child_set.creating =
                consume(TokenKind::Identifier, "expected child_set creating field", diagnostics)
                    .lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "succeeded"))
        {
            advance();
            child_set.succeeded =
                consume(TokenKind::Identifier, "expected child_set succeeded field", diagnostics)
                    .lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "failed"))
        {
            advance();
            child_set.failed =
                consume(TokenKind::Identifier, "expected child_set failed field", diagnostics)
                    .lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "desired_count"))
        {
            advance();
            child_set.desired_count = parse_simple_expression_until_boundary();
            consume_optional_semicolon();
        }
        else if (check(TokenKind::KeywordAnnotations))
        {
            advance();
            skip_balanced_block();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after child_set block", diagnostics);

    child_set.range = SourceRange{start.range.begin, previous().range.end};
    return child_set;
}

WorkflowStepDecl Parser::parse_workflow_step_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordStep, "expected workflow step", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected workflow step name", diagnostics);
    WorkflowStepDecl step;
    step.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after workflow step name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (is_named_identifier(peek(), SyntaxIdentifierExpectedExecutionTime))
        {
            advance();
            const auto value =
                consume(TokenKind::DurationLiteral, "expected step duration", diagnostics);
            step.expected_execution_time = value.lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "max_retries"))
        {
            advance();
            step.max_retries = parse_int_or_zero(
                consume(TokenKind::IntegerLiteral, "expected max_retries integer", diagnostics)
            );
            consume_optional_semicolon();
        }
        else if (check_any({TokenKind::KeywordRequire,      TokenKind::KeywordSet,
                            TokenKind::KeywordEmit,         TokenKind::KeywordEnqueue,
                            TokenKind::KeywordAcquire,      TokenKind::KeywordRenew,
                            TokenKind::KeywordRelease,      TokenKind::KeywordStart,
                            TokenKind::KeywordTransitionTo, TokenKind::KeywordComplete,
                            TokenKind::KeywordFail,         TokenKind::KeywordAtomic,
                            TokenKind::KeywordForEach,      TokenKind::KeywordWhen,
                            TokenKind::KeywordCreate,       TokenKind::KeywordObserve,
                            TokenKind::KeywordMove,         TokenKind::KeywordReserve,
                            TokenKind::KeywordMaterialize,  TokenKind::KeywordReconcile}))
        {
            step.statements.push_back(parse_workflow_statement_decl(diagnostics));
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after workflow step block", diagnostics);

    step.range = SourceRange{start.range.begin, previous().range.end};
    return step;
}

WorkflowStatementDecl Parser::parse_workflow_statement_decl(DiagnosticBag& diagnostics)
{
    const auto start = advance();
    WorkflowStatementDecl statement;

    switch (start.kind)
    {
    case TokenKind::KeywordRequire:
        statement.kind = "require";
        statement.expression = parse_simple_expression_until_boundary();
        break;
    case TokenKind::KeywordSet:
        statement.kind = "set";
        statement.assignable = parse_expression_until(TokenKind::Equals);
        consume(TokenKind::Equals, "expected '=' in set statement", diagnostics);
        statement.expression = parse_simple_expression_until_boundary();
        break;
    case TokenKind::KeywordEmit:
        statement.kind = "emit";
        statement.target = parse_qualified_name(diagnostics, "emitted event");
        statement.payload = parse_workflow_payload(diagnostics);
        break;
    case TokenKind::KeywordEnqueue:
        statement.kind = "enqueue";
        statement.target = parse_qualified_name(diagnostics, "enqueued message");
        statement.payload = parse_workflow_payload(diagnostics);
        break;
    case TokenKind::KeywordAcquire:
        statement.kind = "acquire_lease";
        consume(TokenKind::KeywordLease, "expected lease after acquire", diagnostics);
        statement.target = parse_qualified_name(diagnostics, "acquired lease");
        if (match(TokenKind::KeywordAs))
        {
            statement.binding =
                consume(TokenKind::Identifier, "expected lease binding", diagnostics).lexeme;
        }
        break;
    case TokenKind::KeywordRenew:
        statement.kind = "renew_lease";
        consume(TokenKind::KeywordLease, "expected lease after renew", diagnostics);
        statement.target = parse_qualified_name(diagnostics, "renewed lease");
        break;
    case TokenKind::KeywordRelease:
        statement.kind = "release_lease";
        consume(TokenKind::KeywordLease, "expected lease after release", diagnostics);
        statement.target = parse_qualified_name(diagnostics, "released lease");
        break;
    case TokenKind::KeywordStart:
        statement.kind = "start_workflow";
        consume(TokenKind::KeywordWorkflow, "expected workflow after start", diagnostics);
        statement.target = parse_qualified_name(diagnostics, "started workflow");
        statement.payload = parse_workflow_payload(diagnostics);
        break;
    case TokenKind::KeywordTransitionTo:
        statement.kind = "transition_to";
        statement.target =
            consume(TokenKind::Identifier, "expected transition_to target", diagnostics).lexeme;
        break;
    case TokenKind::KeywordComplete:
        statement.kind = "complete";
        break;
    case TokenKind::KeywordFail:
        statement.kind = "fail";
        if (!check(TokenKind::Semicolon) && !check(TokenKind::RightBrace))
        {
            statement.expression = parse_simple_expression_until_boundary();
        }
        break;
    case TokenKind::KeywordAtomic:
        statement.kind = "atomic";
        statement.statements = parse_workflow_statement_block(diagnostics);
        break;
    case TokenKind::KeywordForEach:
        statement.kind = "for_each";
        statement.binding =
            consume(TokenKind::Identifier, "expected for_each iterator name", diagnostics).lexeme;
        consume(TokenKind::KeywordIn, "expected in after for_each iterator", diagnostics);
        statement.expression = parse_expression_until(TokenKind::LeftBrace);
        statement.statements = parse_workflow_statement_block(diagnostics);
        break;
    case TokenKind::KeywordWhen:
        statement.kind = "when";
        statement.expression = parse_expression_until(TokenKind::LeftBrace);
        statement.statements = parse_workflow_statement_block(diagnostics);
        break;
    case TokenKind::KeywordCreate:
        statement.kind = "create_child";
        consume(TokenKind::KeywordChild, "expected child after create", diagnostics);
        statement.target = parse_qualified_name(diagnostics, "created child entity");
        statement.payload = parse_workflow_payload(diagnostics);
        break;
    case TokenKind::KeywordObserve:
        statement.kind = "observe_child";
        consume(TokenKind::KeywordChild, "expected child after observe", diagnostics);
        statement.target = parse_qualified_name(diagnostics, "observed child entity");
        if (!is_named_identifier(peek(), SyntaxIdentifierBy))
        {
            diagnostics.error(peek().range, "SSPEC0200", "expected by in observe child");
        }
        else
        {
            advance();
        }
        statement.binding =
            consume(TokenKind::Identifier, "expected observe child field", diagnostics).lexeme;
        consume(TokenKind::Equals, "expected '=' in observe child", diagnostics);
        statement.expression = parse_simple_expression_until_boundary();
        break;
    case TokenKind::KeywordMove:
        statement.kind = "move";
        statement.expression = parse_expression_until(TokenKind::KeywordFrom);
        consume(TokenKind::KeywordFrom, "expected from in move statement", diagnostics);
        statement.from_assignable = parse_expression_until(TokenKind::KeywordTo);
        consume(TokenKind::KeywordTo, "expected to in move statement", diagnostics);
        statement.to_assignable = parse_simple_expression_until_boundary();
        break;
    case TokenKind::KeywordReserve:
        statement.kind = "reserve_child_set";
        consume(TokenKind::KeywordChildSet, "expected child_set after reserve", diagnostics);
        statement.target =
            consume(TokenKind::Identifier, "expected child_set name", diagnostics).lexeme;
        break;
    case TokenKind::KeywordMaterialize:
        statement.kind = "materialize_child_set";
        consume(TokenKind::KeywordChildSet, "expected child_set after materialize", diagnostics);
        statement.target =
            consume(TokenKind::Identifier, "expected child_set name", diagnostics).lexeme;
        break;
    case TokenKind::KeywordReconcile:
        statement.kind = "reconcile_child_set";
        consume(TokenKind::KeywordChildSet, "expected child_set after reconcile", diagnostics);
        statement.target =
            consume(TokenKind::Identifier, "expected child_set name", diagnostics).lexeme;
        break;
    default:
        statement.kind = "unknown";
        synchronize_to_member_boundary();
        break;
    }

    consume_optional_semicolon();
    statement.range = SourceRange{start.range.begin, previous().range.end};
    return statement;
}

std::vector<WorkflowStatementDecl>
Parser::parse_workflow_statement_block(DiagnosticBag& diagnostics)
{
    std::vector<WorkflowStatementDecl> statements;
    consume(TokenKind::LeftBrace, "expected workflow statement block", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (check_any({TokenKind::KeywordRequire,      TokenKind::KeywordSet,
                       TokenKind::KeywordEmit,         TokenKind::KeywordEnqueue,
                       TokenKind::KeywordAcquire,      TokenKind::KeywordRenew,
                       TokenKind::KeywordRelease,      TokenKind::KeywordStart,
                       TokenKind::KeywordTransitionTo, TokenKind::KeywordComplete,
                       TokenKind::KeywordFail,         TokenKind::KeywordAtomic,
                       TokenKind::KeywordForEach,      TokenKind::KeywordWhen,
                       TokenKind::KeywordCreate,       TokenKind::KeywordObserve,
                       TokenKind::KeywordMove,         TokenKind::KeywordReserve,
                       TokenKind::KeywordMaterialize,  TokenKind::KeywordReconcile}))
        {
            statements.push_back(parse_workflow_statement_decl(diagnostics));
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after workflow statement block", diagnostics);
    return statements;
}

std::vector<WorkflowAssignmentDecl> Parser::parse_workflow_payload(DiagnosticBag& diagnostics)
{
    std::vector<WorkflowAssignmentDecl> payload;
    if (!match(TokenKind::LeftBrace))
    {
        return payload;
    }

    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        Token name;
        if (check(TokenKind::KeywordKey) || check(TokenKind::KeywordParent))
        {
            name = advance();
        }
        else
        {
            name = consume(TokenKind::Identifier, "expected payload field", diagnostics);
        }
        consume(TokenKind::Equals, "expected '=' in payload assignment", diagnostics);
        WorkflowAssignmentDecl assignment;
        assignment.name = name.lexeme;
        assignment.expression = parse_simple_expression_until_boundary();
        consume_optional_semicolon();
        assignment.range = SourceRange{name.range.begin, previous().range.end};
        payload.push_back(std::move(assignment));
    }

    consume(TokenKind::RightBrace, "expected '}' after workflow payload", diagnostics);
    return payload;
}

} // namespace statespec
