#include "statespec/parser.hpp"

#include "parser_helpers.hpp"

#include <utility>

namespace statespec
{

using parser_detail::is_named_identifier;
using parser_detail::qualify_decl_name;
using parser_detail::qualify_name;
using parser_detail::strip_quotes;

Spec Parser::parse_spec(DiagnosticBag& diagnostics)
{
    Spec spec;

    if (match(TokenKind::KeywordStatespec))
    {
        const auto version =
            consume(TokenKind::DecimalLiteral, "expected StateSpec version", diagnostics);
        spec.version = version.lexeme;
        consume(TokenKind::Semicolon, "expected ';' after StateSpec version", diagnostics);
    }

    while (match(TokenKind::KeywordInclude))
    {
        rewind_one();
        spec.includes.push_back(parse_include_decl(diagnostics));
    }

    if (check(TokenKind::KeywordSystem))
    {
        spec.system = parse_system_decl(diagnostics);
    }
    else
    {
        diagnostics.error(peek().range, "SSPEC0201", "expected system declaration");
    }

    while (!is_at_end())
    {
        diagnostics.error(peek().range, "SSPEC0202", "unexpected token after system declaration");
        advance();
    }

    return spec;
}

IncludeDecl Parser::parse_include_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordInclude, "expected include declaration", diagnostics);
    const auto path = consume(TokenKind::StringLiteral, "expected include path", diagnostics);
    consume(TokenKind::Semicolon, "expected ';' after include declaration", diagnostics);

    IncludeDecl include_decl;
    include_decl.path = strip_quotes(path.lexeme);
    include_decl.range = SourceRange{start.range.begin, previous().range.end};
    return include_decl;
}

SystemDecl Parser::parse_system_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordSystem, "expected system declaration", diagnostics);
    auto name = consume(TokenKind::Identifier, "expected system name", diagnostics);
    SystemDecl system;
    system.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after system name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordTenant))
        {
            const auto tenant_start = previous();
            system.member_order.push_back(BlockMemberOrder{"tenant", tenant_start.range});
            consume(TokenKind::KeywordScopedBy, "expected scoped_by after tenant", diagnostics);
            TenantScopeDecl tenant_scope;
            tenant_scope.field_name =
                consume(TokenKind::Identifier, "expected tenant scope field", diagnostics).lexeme;
            tenant_scope.range = SourceRange{tenant_start.range.begin, previous().range.end};
            system.tenant_scope = tenant_scope;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "system_tenant"))
        {
            const auto system_tenant_start = advance();
            system.member_order.push_back(
                BlockMemberOrder{"system_tenant", system_tenant_start.range}
            );
            if (!is_named_identifier(peek(), "configured"))
            {
                diagnostics.error(
                    peek().range, "SSPEC0200", "expected configured after system_tenant"
                );
            }
            else
            {
                advance();
            }
            SystemTenantDecl system_tenant;
            system_tenant.range =
                SourceRange{system_tenant_start.range.begin, previous().range.end};
            system.system_tenant = system_tenant;
            consume_optional_semicolon();
        }
        else if (check(TokenKind::KeywordNamespace))
        {
            system.member_order.push_back(BlockMemberOrder{"namespace", peek().range});
            system.namespaces.push_back(parse_namespace_decl(diagnostics, system));
        }
        else if (check(TokenKind::KeywordEntity))
        {
            system.member_order.push_back(BlockMemberOrder{"entity", peek().range});
            system.entities.push_back(parse_entity_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordValue))
        {
            system.member_order.push_back(BlockMemberOrder{"value", peek().range});
            system.values.push_back(parse_value_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordEnum))
        {
            system.member_order.push_back(BlockMemberOrder{"enum", peek().range});
            system.enums.push_back(parse_enum_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordEvent))
        {
            system.member_order.push_back(BlockMemberOrder{"event", peek().range});
            system.events.push_back(parse_event_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordShape))
        {
            system.member_order.push_back(BlockMemberOrder{"shape", peek().range});
            system.shapes.push_back(parse_shape_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordExternalSystem))
        {
            system.member_order.push_back(BlockMemberOrder{"external_system", peek().range});
            system.external_systems.push_back(parse_external_system_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordFeatureFlag))
        {
            system.member_order.push_back(BlockMemberOrder{"feature_flag", peek().range});
            system.feature_flags.push_back(parse_feature_flag_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordLog))
        {
            system.member_order.push_back(BlockMemberOrder{"log", peek().range});
            system.logs.push_back(parse_log_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordMetric))
        {
            system.member_order.push_back(BlockMemberOrder{"metric", peek().range});
            system.metrics.push_back(parse_metric_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordQueue))
        {
            system.member_order.push_back(BlockMemberOrder{"queue", peek().range});
            system.queues.push_back(parse_queue_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordLease))
        {
            system.member_order.push_back(BlockMemberOrder{"lease", peek().range});
            system.leases.push_back(parse_lease_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordWorker))
        {
            system.member_order.push_back(BlockMemberOrder{"worker", peek().range});
            system.workers.push_back(parse_worker_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordApiServer))
        {
            system.member_order.push_back(BlockMemberOrder{"api_server", peek().range});
            system.api_servers.push_back(parse_api_server_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordApi))
        {
            system.member_order.push_back(BlockMemberOrder{"api", peek().range});
            system.apis.push_back(parse_api_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordWorkflow))
        {
            system.member_order.push_back(BlockMemberOrder{"workflow", peek().range});
            system.workflows.push_back(parse_workflow_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordPolicy))
        {
            system.member_order.push_back(BlockMemberOrder{"policy", peek().range});
            system.policies.push_back(parse_policy_decl(diagnostics));
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after system block", diagnostics);

    system.range = SourceRange{start.range.begin, previous().range.end};
    return system;
}

NamespaceDecl Parser::parse_namespace_decl(
    DiagnosticBag& diagnostics,
    SystemDecl& system,
    const std::string& prefix
)
{
    const auto start =
        consume(TokenKind::KeywordNamespace, "expected namespace declaration", diagnostics);
    const auto name = parse_qualified_name(diagnostics, "namespace name");
    const auto namespace_name = qualify_name(prefix, name);
    NamespaceDecl namespace_decl;
    namespace_decl.name = namespace_name;

    consume(TokenKind::LeftBrace, "expected '{' after namespace name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (check(TokenKind::KeywordNamespace))
        {
            auto nested = parse_namespace_decl(diagnostics, system, namespace_name);
            namespace_decl.members.push_back(nested.name);
            system.namespaces.push_back(std::move(nested));
        }
        else if (check(TokenKind::KeywordEntity))
        {
            auto decl = parse_entity_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.entities.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordValue))
        {
            auto decl = parse_value_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.values.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordEnum))
        {
            auto decl = parse_enum_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.enums.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordEvent))
        {
            auto decl = parse_event_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.events.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordShape))
        {
            auto decl = parse_shape_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.shapes.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordExternalSystem))
        {
            auto decl = parse_external_system_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.external_systems.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordFeatureFlag))
        {
            auto decl = parse_feature_flag_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.feature_flags.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordLog))
        {
            auto decl = parse_log_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.logs.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordMetric))
        {
            auto decl = parse_metric_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.metrics.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordQueue))
        {
            auto decl = parse_queue_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.queues.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordLease))
        {
            auto decl = parse_lease_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.leases.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordWorker))
        {
            auto decl = parse_worker_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.workers.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordApiServer))
        {
            auto decl = parse_api_server_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.api_servers.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordApi))
        {
            auto decl = parse_api_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.apis.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordWorkflow))
        {
            auto decl = parse_workflow_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.workflows.push_back(std::move(decl));
        }
        else if (check(TokenKind::KeywordPolicy))
        {
            auto decl = parse_policy_decl(diagnostics);
            qualify_decl_name(decl, namespace_name);
            namespace_decl.members.push_back(decl.name);
            system.policies.push_back(std::move(decl));
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after namespace block", diagnostics);

    namespace_decl.range = SourceRange{start.range.begin, previous().range.end};
    return namespace_decl;
}

} // namespace statespec
