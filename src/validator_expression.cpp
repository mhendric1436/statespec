#include "validator_declarations.hpp"

#include "statespec/lexer.hpp"

#include "validator_helpers.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <utility>
#include <vector>

namespace statespec::validator_detail
{

namespace
{

const FeatureFlagDecl* find_feature_flag(
    const SystemDecl& system,
    const std::string& name
)
{
    for (const auto& flag : system.feature_flags)
    {
        if (flag.name == name)
        {
            return &flag;
        }
    }
    return nullptr;
}

bool is_expression_identifier_token(TokenKind kind)
{
    switch (kind)
    {
    case TokenKind::Identifier:
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
    case TokenKind::KeywordEvent:
    case TokenKind::KeywordQueue:
    case TokenKind::KeywordMessage:
    case TokenKind::KeywordLease:
    case TokenKind::KeywordWorker:
    case TokenKind::KeywordApiServer:
    case TokenKind::KeywordApi:
    case TokenKind::KeywordWorkflow:
    case TokenKind::KeywordStep:
    case TokenKind::KeywordChildSet:
    case TokenKind::KeywordPolicy:
    case TokenKind::KeywordTenant:
    case TokenKind::KeywordInput:
    case TokenKind::KeywordOutput:
    case TokenKind::KeywordError:
    case TokenKind::KeywordStarts:
    case TokenKind::KeywordEnqueues:
    case TokenKind::KeywordPolls:
    case TokenKind::KeywordExecutes:
    case TokenKind::KeywordSingleton:
    case TokenKind::KeywordConcurrency:
        return true;
    default:
        return false;
    }
}

bool is_allowed_expression_builtin(const std::string& name)
{
    static const std::unordered_set<std::string> builtins{
        "count",         "empty",           "contains",      "disjoint",     "exists",
        "current_state", "can_transition",  "changed",       "generate_ids", "ordinal_of",
        "child_state",   "all_children",    "any_child",     "now",          "duration",
        "expired",       "feature_enabled", "feature_value",
    };
    return builtins.find(name) != builtins.end();
}

class ExpressionSyntaxValidator
{
  public:
    ExpressionSyntaxValidator(
        const SourceRange& range,
        std::vector<Token> tokens,
        DiagnosticBag& diagnostics
    )
        : range_(range),
          tokens_(std::move(tokens)),
          diagnostics_(diagnostics)
    {
    }

    void validate()
    {
        if (check(TokenKind::EndOfFile))
        {
            syntax_error("expression must not be empty");
            return;
        }
        parse_expression();
        if (!failed_ && !check(TokenKind::EndOfFile))
        {
            syntax_error("unexpected token '" + peek().lexeme + "' in expression");
        }
    }

  private:
    void parse_expression()
    {
        parse_logical_or();
    }

    void parse_logical_or()
    {
        parse_logical_and();
        while (match(TokenKind::OrOr))
        {
            parse_logical_and();
        }
    }

    void parse_logical_and()
    {
        parse_equality();
        while (match(TokenKind::AndAnd))
        {
            parse_equality();
        }
    }

    void parse_equality()
    {
        parse_comparison();
        while (match(TokenKind::EqualEqual) || match(TokenKind::BangEqual))
        {
            parse_comparison();
        }
    }

    void parse_comparison()
    {
        parse_additive();
        while (match(TokenKind::Less) || match(TokenKind::LessEqual) || match(TokenKind::Greater) ||
               match(TokenKind::GreaterEqual) || match(TokenKind::KeywordIn))
        {
            parse_additive();
        }
    }

    void parse_additive()
    {
        parse_multiplicative();
        while (match(TokenKind::Plus) || match(TokenKind::Minus))
        {
            parse_multiplicative();
        }
    }

    void parse_multiplicative()
    {
        parse_unary();
        while (match(TokenKind::Star) || match(TokenKind::Slash) || match(TokenKind::Percent))
        {
            parse_unary();
        }
    }

    void parse_unary()
    {
        if (match(TokenKind::Bang) || match(TokenKind::Minus))
        {
            parse_unary();
            return;
        }
        parse_primary();
    }

    void parse_primary()
    {
        if (match(TokenKind::StringLiteral) || match(TokenKind::IntegerLiteral) ||
            match(TokenKind::DecimalLiteral) || match(TokenKind::BooleanLiteral) ||
            match(TokenKind::DurationLiteral))
        {
            return;
        }
        if (match(TokenKind::LeftParen))
        {
            parse_expression();
            consume(TokenKind::RightParen, "expected ')' after expression");
            return;
        }
        if (match(TokenKind::LeftBracket))
        {
            if (!check(TokenKind::RightBracket))
            {
                parse_expression();
                while (match(TokenKind::Comma))
                {
                    parse_expression();
                }
            }
            consume(TokenKind::RightBracket, "expected ']' after list expression");
            return;
        }
        if (is_expression_identifier_token(peek().kind))
        {
            const auto name = parse_qualified_name();
            if (match(TokenKind::LeftParen))
            {
                validate_builtin_call(name);
                if (!check(TokenKind::RightParen))
                {
                    parse_expression();
                    while (match(TokenKind::Comma))
                    {
                        parse_expression();
                    }
                }
                consume(TokenKind::RightParen, "expected ')' after function arguments");
            }
            parse_selectors();
            return;
        }
        syntax_error("expected expression");
    }

    void parse_selectors()
    {
        while (true)
        {
            if (match(TokenKind::Dot))
            {
                if (!is_expression_identifier_token(peek().kind))
                {
                    syntax_error("expected selector name after '.'");
                    return;
                }
                advance();
            }
            else if (match(TokenKind::LeftBracket))
            {
                parse_expression();
                consume(TokenKind::RightBracket, "expected ']' after selector expression");
            }
            else
            {
                return;
            }
        }
    }

    std::string parse_qualified_name()
    {
        std::string name = advance().lexeme;
        while (match(TokenKind::Dot))
        {
            if (!is_expression_identifier_token(peek().kind))
            {
                syntax_error("expected name after '.'");
                return name;
            }
            name += ".";
            name += advance().lexeme;
        }
        return name;
    }

    void validate_builtin_call(const std::string& name)
    {
        if (!is_allowed_expression_builtin(name))
        {
            diagnostics_.error(
                range_, "SSPEC5202", "expression function '" + name + "' is not an allowed built-in"
            );
        }
    }

    void consume(
        TokenKind kind,
        const std::string& message
    )
    {
        if (check(kind))
        {
            advance();
            return;
        }
        syntax_error(message);
    }

    bool match(TokenKind kind)
    {
        if (!check(kind))
        {
            return false;
        }
        advance();
        return true;
    }

    bool check(TokenKind kind) const
    {
        return peek().kind == kind;
    }

    const Token& peek() const
    {
        return tokens_[current_];
    }

    const Token& advance()
    {
        if (!check(TokenKind::EndOfFile))
        {
            ++current_;
        }
        return tokens_[current_ - 1];
    }

    void syntax_error(const std::string& message)
    {
        if (failed_)
        {
            return;
        }
        failed_ = true;
        diagnostics_.error(range_, "SSPEC5201", "invalid expression: " + message);
    }

    SourceRange range_;
    std::vector<Token> tokens_;
    DiagnosticBag& diagnostics_;
    std::size_t current_ = 0;
    bool failed_ = false;
};

std::vector<std::string> feature_flag_function_arguments(
    const std::string& expression,
    const std::string& function_name
)
{
    std::vector<std::string> arguments;
    std::size_t offset = 0;

    while ((offset = expression.find(function_name, offset)) != std::string::npos)
    {
        auto cursor = offset + function_name.size();
        while (cursor < expression.size() &&
               std::isspace(static_cast<unsigned char>(expression[cursor])) != 0)
        {
            ++cursor;
        }
        if (cursor >= expression.size() || expression[cursor] != '(')
        {
            offset = cursor;
            continue;
        }

        const auto arg_start = cursor + 1;
        const auto arg_end = expression.find(')', arg_start);
        if (arg_end == std::string::npos)
        {
            break;
        }

        auto argument = expression.substr(arg_start, arg_end - arg_start);
        argument.erase(
            std::remove_if(
                argument.begin(), argument.end(),
                [](char ch) { return std::isspace(static_cast<unsigned char>(ch)) != 0; }
            ),
            argument.end()
        );
        arguments.push_back(argument);
        offset = arg_end + 1;
    }

    return arguments;
}
} // namespace

void validate_feature_flag_expression(
    const SystemDecl& system,
    const SourceRange& range,
    const std::string& expression,
    DiagnosticBag& diagnostics
)
{
    for (const auto& flag_name : feature_flag_function_arguments(expression, "feature_enabled"))
    {
        const auto* flag = find_feature_flag(system, flag_name);
        if (flag == nullptr)
        {
            unknown_reference_error(diagnostics, range, "feature flag", flag_name);
            continue;
        }
        if (flag->type.value_or("") != "bool")
        {
            diagnostics.error(
                range, "SSPEC4204", "feature_enabled requires bool feature flag '" + flag_name + "'"
            );
        }
    }

    for (const auto& flag_name : feature_flag_function_arguments(expression, "feature_value"))
    {
        if (find_feature_flag(system, flag_name) == nullptr)
        {
            unknown_reference_error(diagnostics, range, "feature flag", flag_name);
        }
    }
}

void validate_expression(
    const SystemDecl& system,
    const SourceRange& range,
    const std::string& expression,
    DiagnosticBag& diagnostics
)
{
    SourceFile source{"<expression>", expression};
    Lexer lexer{source};
    DiagnosticBag expression_diagnostics;
    auto tokens = lexer.lex(expression_diagnostics);
    if (expression_diagnostics.has_errors())
    {
        diagnostics.error(range, "SSPEC5201", "invalid expression: lexer rejected expression");
        return;
    }

    ExpressionSyntaxValidator validator{range, std::move(tokens), diagnostics};
    validator.validate();
    validate_feature_flag_expression(system, range, expression, diagnostics);
}
} // namespace statespec::validator_detail
