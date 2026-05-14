#pragma once

#include "statespec/ast.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/token.hpp"

#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>

namespace statespec
{

class Parser
{
  public:
    explicit Parser(std::vector<Token> tokens);

    Spec parse(DiagnosticBag& diagnostics);

  private:
    const Token& peek(std::size_t offset = 0) const;
    const Token& previous() const;
    bool is_at_end() const;
    bool check(TokenKind kind) const;
    bool check_any(std::initializer_list<TokenKind> kinds) const;
    bool match(TokenKind kind);
    bool match_any(std::initializer_list<TokenKind> kinds);
    Token advance();
    Token consume(
        TokenKind kind,
        const std::string& message,
        DiagnosticBag& diagnostics
    );

    Spec parse_spec(DiagnosticBag& diagnostics);
    ImportDecl parse_import_decl(DiagnosticBag& diagnostics);
    SystemDecl parse_system_decl(DiagnosticBag& diagnostics);
    FeatureFlagDecl parse_feature_flag_decl(DiagnosticBag& diagnostics);
    EntityDecl parse_entity_decl(DiagnosticBag& diagnostics);
    QueueDecl parse_queue_decl(DiagnosticBag& diagnostics);
    MessageDecl parse_message_decl(DiagnosticBag& diagnostics);
    LeaseDecl parse_lease_decl(DiagnosticBag& diagnostics);
    WorkerDecl parse_worker_decl(DiagnosticBag& diagnostics);
    ApiDecl parse_api_decl(DiagnosticBag& diagnostics);
    WorkflowDecl parse_workflow_decl(DiagnosticBag& diagnostics);
    WorkflowStepDecl parse_workflow_step_decl(DiagnosticBag& diagnostics);
    PolicyDecl parse_policy_decl(DiagnosticBag& diagnostics);
    StateMachineDecl parse_state_machine_decl(DiagnosticBag& diagnostics);
    StateDecl parse_state_decl(DiagnosticBag& diagnostics);
    GarbageCollectionPolicyDecl parse_garbage_collection_policy_decl(DiagnosticBag& diagnostics);
    FieldDecl parse_field_decl(DiagnosticBag& diagnostics);
    IndexDecl parse_index_decl(DiagnosticBag& diagnostics);

    std::string parse_qualified_name(
        DiagnosticBag& diagnostics,
        const std::string& context
    );
    std::string parse_type_name(DiagnosticBag& diagnostics);
    std::vector<std::string> parse_identifier_list(DiagnosticBag& diagnostics);
    std::string parse_simple_value(
        DiagnosticBag& diagnostics,
        const std::string& context
    );
    std::string parse_simple_expression_until_boundary();

    void consume_optional_semicolon();
    void skip_unknown_declaration(DiagnosticBag& diagnostics);
    void skip_balanced_block();
    void synchronize_to_member_boundary();

    std::vector<Token> tokens_;
    std::size_t current_ = 0;
};

} // namespace statespec
