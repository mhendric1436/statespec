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

int policy_member_order_index(const std::string& kind)
{
    if (kind == "tenant")
    {
        return 0;
    }
    if (kind == "allow")
    {
        return 1;
    }
    if (kind == "deny")
    {
        return 2;
    }
    if (kind == "quota")
    {
        return 3;
    }
    if (kind == "audit")
    {
        return 4;
    }
    return 5;
}

int log_member_order_index(const std::string& kind)
{
    if (kind == "level")
    {
        return 0;
    }
    if (kind == "event_name")
    {
        return 1;
    }
    if (kind == "fields")
    {
        return 2;
    }
    return 3;
}

int metric_member_order_index(const std::string& kind)
{
    if (kind == "kind")
    {
        return 0;
    }
    if (kind == "name")
    {
        return 1;
    }
    if (kind == "unit")
    {
        return 2;
    }
    if (kind == "labels")
    {
        return 3;
    }
    return 4;
}

int system_member_order_index(const std::string& kind)
{
    if (kind == "tenant")
    {
        return 0;
    }
    if (kind == "system_tenant")
    {
        return 1;
    }
    if (kind == "namespace")
    {
        return 2;
    }
    if (kind == "value" || kind == "enum" || kind == "shape" || kind == "external_system" ||
        kind == "event")
    {
        return 3;
    }
    if (kind == "feature_flag")
    {
        return 4;
    }
    if (kind == "log" || kind == "metric")
    {
        return 5;
    }
    if (kind == "entity")
    {
        return 6;
    }
    if (kind == "queue")
    {
        return 7;
    }
    if (kind == "lease")
    {
        return 8;
    }
    if (kind == "worker")
    {
        return 9;
    }
    if (kind == "workflow")
    {
        return 10;
    }
    if (kind == "api_server" || kind == "api")
    {
        return 11;
    }
    if (kind == "policy")
    {
        return 12;
    }
    return 13;
}

void validate_policy_member_order(
    const PolicyDecl& policy,
    DiagnosticBag& diagnostics
)
{
    int previous_order = -1;
    for (const auto& member : policy.member_order)
    {
        const auto order = policy_member_order_index(member.kind);
        if (order < previous_order)
        {
            diagnostics.warning(
                member.range, "SSPEC6103",
                "policy '" + policy.name +
                    "' members should use canonical order: tenant, allow, deny, quota, audit"
            );
            return;
        }
        previous_order = order;
    }
}

void validate_system_member_order_impl(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    int previous_order = -1;
    for (const auto& member : system.member_order)
    {
        const auto order = system_member_order_index(member.kind);
        if (order < previous_order)
        {
            diagnostics.warning(
                member.range, "SSPEC6106",
                "system '" + system.name +
                    "' members should use canonical order: tenant, system_tenant, "
                    "namespace, values/enums/shapes/events, feature_flags, logs/metrics, "
                    "entities, queues, leases, workers, workflows, APIs, policies"
            );
            return;
        }
        previous_order = order;
    }
}

void validate_log_member_order(
    const LogDecl& log,
    DiagnosticBag& diagnostics
)
{
    int previous_order = -1;
    for (const auto& member : log.member_order)
    {
        const auto order = log_member_order_index(member.kind);
        if (order < previous_order)
        {
            diagnostics.warning(
                member.range, "SSPEC6104",
                "log '" + log.name +
                    "' members should use canonical order: level, event_name, fields"
            );
            return;
        }
        previous_order = order;
    }
}

void validate_metric_member_order(
    const MetricDecl& metric,
    DiagnosticBag& diagnostics
)
{
    int previous_order = -1;
    for (const auto& member : metric.member_order)
    {
        const auto order = metric_member_order_index(member.kind);
        if (order < previous_order)
        {
            diagnostics.warning(
                member.range, "SSPEC6105",
                "metric '" + metric.name +
                    "' members should use canonical order: kind, name, unit, labels"
            );
            return;
        }
        previous_order = order;
    }
}

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

std::string lowercase_copy(const std::string& value)
{
    std::string result;
    result.reserve(value.size());
    for (const auto ch : value)
    {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return result;
}

bool ends_with(
    const std::string& value,
    const std::string& suffix
)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool is_high_cardinality_metric_label_name(
    const SystemDecl& system,
    const std::string& label_name
)
{
    if (label_name == "tenant_id")
    {
        return false;
    }
    if (system.tenant_scope.has_value() && label_name == system.tenant_scope->field_name)
    {
        return false;
    }

    const auto name = lowercase_copy(label_name);
    return name == "id" || name == "uuid" || name == "guid" || ends_with(name, "_id") ||
           ends_with(name, "_ids") || ends_with(name, "_uuid") || ends_with(name, "_guid") ||
           ends_with(name, "_at") || ends_with(name, "_time") || ends_with(name, "_timestamp") ||
           name.find("payload") != std::string::npos ||
           name.find("error_message") != std::string::npos;
}

void validate_log_tenant_field(
    const SystemDecl& system,
    const LogDecl& log,
    DiagnosticBag& diagnostics
)
{
    if (!system.tenant_scope.has_value())
    {
        return;
    }

    const auto fields = field_names(log.fields);
    if (!contains(fields, system.tenant_scope->field_name))
    {
        diagnostics.error(
            log.range, "SSPEC3407",
            "log '" + log.name + "' fields must declare tenant field '" +
                system.tenant_scope->field_name + "'"
        );
    }
}

void validate_metric_tenant_label(
    const SystemDecl& system,
    const MetricDecl& metric,
    DiagnosticBag& diagnostics
)
{
    if (!system.tenant_scope.has_value())
    {
        return;
    }

    const auto labels = field_names(metric.labels);
    if (!contains(labels, system.tenant_scope->field_name))
    {
        diagnostics.error(
            metric.range, "SSPEC3408",
            "metric '" + metric.name + "' labels must declare tenant field '" +
                system.tenant_scope->field_name + "'"
        );
    }
}

} // namespace

void validate_system_member_order(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    validate_system_member_order_impl(system, diagnostics);
}

void validate_field_duplicates(
    const std::vector<FieldDecl>& fields,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> names;
    for (const auto& field : fields)
    {
        if (!names.insert(field.name).second)
        {
            duplicate_error(diagnostics, field.range, field.name);
        }
    }
}

void validate_field_types(
    const std::vector<FieldDecl>& fields,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& field : fields)
    {
        const auto base_type = base_type_name(field.type);
        if (!is_builtin_type(base_type) && !symbols.find(base_type).has_value())
        {
            unknown_reference_error(diagnostics, field.range, "type", base_type);
        }
    }
}

bool is_known_type_reference(
    const std::string& type,
    const SymbolTable& symbols
)
{
    const auto base_type = base_type_name(type);
    return is_builtin_type(base_type) || symbols.find(base_type).has_value();
}

void validate_system_tenancy(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    if (!system.tenant_scope.has_value())
    {
        required_error(
            diagnostics, system.range, "system '" + system.name + "'", "tenant scoped_by"
        );
    }
    if (!system.system_tenant.has_value())
    {
        required_error(
            diagnostics, system.range, "system '" + system.name + "'", "system_tenant configured"
        );
    }
}

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

void validate_shapes(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& shape : system.shapes)
    {
        if (!is_qualified_pascal_case_name(shape.name))
        {
            diagnostics.error(
                shape.range, "SSPEC3201", "shape '" + shape.name + "' must use PascalCase"
            );
        }
        if (shape.fields.empty())
        {
            required_error(diagnostics, shape.range, "shape '" + shape.name + "'", "fields");
        }
        validate_field_duplicates(shape.fields, diagnostics);
        validate_field_types(shape.fields, symbols, diagnostics);
    }
}

void validate_values(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& value : system.values)
    {
        if (!is_qualified_pascal_case_name(value.name))
        {
            diagnostics.error(
                value.range, "SSPEC4501", "value '" + value.name + "' must use PascalCase"
            );
        }
        if (value.type.empty())
        {
            required_error(diagnostics, value.range, "value '" + value.name + "'", "type");
        }
        else if (!is_known_type_reference(value.type, symbols))
        {
            unknown_reference_error(diagnostics, value.range, "value type", value.type);
        }
        if (value.constraint.has_value())
        {
            validate_expression(system, value.range, *value.constraint, diagnostics);
        }
    }
}

void validate_namespaces(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& namespace_decl : system.namespaces)
    {
        if (!is_qualified_pascal_case_name(namespace_decl.name))
        {
            diagnostics.error(
                namespace_decl.range, "SSPEC4801",
                "namespace '" + namespace_decl.name + "' must use PascalCase segments"
            );
        }
        for (const auto& member : namespace_decl.members)
        {
            if (!symbols.find(member).has_value())
            {
                unknown_reference_error(
                    diagnostics, namespace_decl.range, "namespace member", member
                );
            }
        }
    }
}

void validate_enums(
    DiagnosticBag& diagnostics,
    const SystemDecl& system
)
{
    for (const auto& enum_decl : system.enums)
    {
        if (!is_qualified_pascal_case_name(enum_decl.name))
        {
            diagnostics.error(
                enum_decl.range, "SSPEC4601", "enum '" + enum_decl.name + "' must use PascalCase"
            );
        }
        if (enum_decl.members.empty())
        {
            required_error(diagnostics, enum_decl.range, "enum '" + enum_decl.name + "'", "member");
        }

        std::unordered_set<std::string> members;
        for (const auto& member : enum_decl.members)
        {
            if (!is_pascal_case_name(member.name))
            {
                diagnostics.error(
                    member.range, "SSPEC4602",
                    "enum member '" + enum_decl.name + "." + member.name + "' must use PascalCase"
                );
            }
            if (!members.insert(member.name).second)
            {
                duplicate_error(diagnostics, member.range, enum_decl.name + "." + member.name);
            }
        }
    }
}

void validate_events(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& event : system.events)
    {
        if (!is_qualified_pascal_case_name(event.name))
        {
            diagnostics.error(
                event.range, "SSPEC4701", "event '" + event.name + "' must use PascalCase"
            );
        }
        if (event.fields.empty())
        {
            required_error(diagnostics, event.range, "event '" + event.name + "'", "fields");
        }
        validate_field_duplicates(event.fields, diagnostics);
        validate_field_types(event.fields, symbols, diagnostics);
    }
}

void validate_external_systems(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    for (const auto& external_system : system.external_systems)
    {
        if (!is_qualified_pascal_case_name(external_system.name))
        {
            diagnostics.error(
                external_system.range, "SSPEC4901",
                "external_system '" + external_system.name + "' must use PascalCase segments"
            );
        }

        std::unordered_set<std::string> properties;
        for (const auto& property : external_system.properties)
        {
            if (!properties.insert(property.name).second)
            {
                duplicate_error(diagnostics, property.range, property.name);
            }
        }
    }
}

void validate_feature_flags(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& flag : system.feature_flags)
    {
        if (!is_qualified_pascal_case_name(flag.name))
        {
            diagnostics.error(
                flag.range, "SSPEC4201", "feature flag '" + flag.name + "' must use PascalCase"
            );
        }

        if (!flag.type.has_value())
        {
            required_error(diagnostics, flag.range, "feature_flag '" + flag.name + "'", "type");
        }
        else if (!is_supported_feature_flag_type(*flag.type))
        {
            diagnostics.error(
                flag.range, "SSPEC4202",
                "feature flag '" + flag.name + "' has unsupported type '" + *flag.type + "'"
            );
        }

        if (!flag.default_value.has_value())
        {
            required_error(diagnostics, flag.range, "feature_flag '" + flag.name + "'", "default");
        }
        else if (!feature_flag_default_matches_type(flag))
        {
            diagnostics.error(
                flag.range, "SSPEC4203",
                "feature flag '" + flag.name + "' default must match declared type"
            );
        }

        if (flag.scope.has_value())
        {
            const auto entity = entity_scope_target(*flag.scope);
            if (entity.has_value())
            {
                const auto symbol = symbols.find(*entity);
                if (!symbol.has_value() || symbol->kind != SymbolKind::Entity)
                {
                    unknown_reference_error(
                        diagnostics, flag.range, "feature flag entity scope", *entity
                    );
                }
            }
        }
    }
}

void validate_logs(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> event_names;
    for (const auto& log : system.logs)
    {
        validate_log_member_order(log, diagnostics);

        if (!is_qualified_pascal_case_name(log.name))
        {
            diagnostics.error(log.range, "SSPEC4301", "log '" + log.name + "' must use PascalCase");
        }

        if (!log.level.has_value())
        {
            required_error(diagnostics, log.range, "log '" + log.name + "'", "level");
        }
        else if (!is_supported_log_level(*log.level))
        {
            diagnostics.error(
                log.range, "SSPEC4302",
                "log '" + log.name + "' has unsupported level '" + *log.level + "'"
            );
        }

        if (!log.event_name.has_value())
        {
            required_error(diagnostics, log.range, "log '" + log.name + "'", "event_name");
        }
        else if (!event_names.insert(*log.event_name).second)
        {
            diagnostics.error(
                log.range, "SSPEC4303", "duplicate log event_name '" + *log.event_name + "'"
            );
        }

        validate_field_duplicates(log.fields, diagnostics);
        validate_field_types(log.fields, symbols, diagnostics);
        validate_log_tenant_field(system, log, diagnostics);
    }
}

void validate_metrics(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> backend_names;
    for (const auto& metric : system.metrics)
    {
        validate_metric_member_order(metric, diagnostics);

        if (!is_qualified_pascal_case_name(metric.name))
        {
            diagnostics.error(
                metric.range, "SSPEC4401", "metric '" + metric.name + "' must use PascalCase"
            );
        }

        if (!metric.kind.has_value())
        {
            required_error(diagnostics, metric.range, "metric '" + metric.name + "'", "kind");
        }
        else if (!is_supported_metric_kind(*metric.kind))
        {
            diagnostics.error(
                metric.range, "SSPEC4402",
                "metric '" + metric.name + "' has unsupported kind '" + *metric.kind + "'"
            );
        }

        if (!metric.backend_name.has_value())
        {
            required_error(diagnostics, metric.range, "metric '" + metric.name + "'", "name");
        }
        else if (!backend_names.insert(*metric.backend_name).second)
        {
            diagnostics.error(
                metric.range, "SSPEC4403", "duplicate metric name '" + *metric.backend_name + "'"
            );
        }

        if (!metric.unit.has_value())
        {
            required_error(diagnostics, metric.range, "metric '" + metric.name + "'", "unit");
        }

        validate_field_duplicates(metric.labels, diagnostics);
        validate_field_types(metric.labels, symbols, diagnostics);
        validate_metric_tenant_label(system, metric, diagnostics);
        for (const auto& label : metric.labels)
        {
            if (!is_supported_metric_label_type(label.type))
            {
                diagnostics.error(
                    label.range, "SSPEC4404",
                    "metric '" + metric.name + "' label '" + label.name +
                        "' must use low-cardinality type string, bool, or int"
                );
            }
            if (is_high_cardinality_metric_label_name(system, label.name))
            {
                diagnostics.error(
                    label.range, "SSPEC4405",
                    "metric '" + metric.name + "' label '" + label.name +
                        "' looks high-cardinality; use a log field or aggregate dimension instead"
                );
            }
        }
    }
}

void validate_policies(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& policy : system.policies)
    {
        validate_policy_member_order(policy, diagnostics);

        if (!policy.tenant_scoped_by.has_value())
        {
            required_error(
                diagnostics, policy.range, "policy '" + policy.name + "'", "tenant scoped_by"
            );
        }
        if (policy.tenant_scoped_by.has_value() && system.tenant_scope.has_value() &&
            *policy.tenant_scoped_by != system.tenant_scope->field_name)
        {
            diagnostics.error(
                policy.range, "SSPEC3405",
                "policy '" + policy.name + "' tenant field '" + *policy.tenant_scoped_by +
                    "' must match system tenant field '" + system.tenant_scope->field_name + "'"
            );
        }
        if (policy.allows.empty() && policy.denies.empty() && policy.quotas.empty() &&
            policy.audits.empty())
        {
            required_error(
                diagnostics, policy.range, "policy '" + policy.name + "'",
                "at least one allow, deny, quota, or audit declaration"
            );
        }

        for (const auto& rule : policy.allows)
        {
            if (!symbols.find(rule.action).has_value())
            {
                unknown_reference_error(
                    diagnostics, rule.range, "policy allow action", rule.action
                );
            }
            validate_expression(system, rule.range, rule.condition, diagnostics);
        }
        for (const auto& rule : policy.denies)
        {
            if (!symbols.find(rule.action).has_value())
            {
                unknown_reference_error(diagnostics, rule.range, "policy deny action", rule.action);
            }
            validate_expression(system, rule.range, rule.condition, diagnostics);
        }
        for (const auto& quota : policy.quotas)
        {
            validate_expression(system, quota.range, quota.expression, diagnostics);
        }
        for (const auto& audit : policy.audits)
        {
            if (!symbols.find(audit).has_value())
            {
                unknown_reference_error(diagnostics, policy.range, "policy audit action", audit);
            }
        }
    }
}

} // namespace statespec::validator_detail
