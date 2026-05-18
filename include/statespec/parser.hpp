#pragma once

#include "statespec/ast.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/parser_context.hpp"

#include <string>
#include <vector>

namespace statespec
{

class Parser : private ParserContext
{
  public:
    explicit Parser(std::vector<Token> tokens);

    Spec parse(DiagnosticBag& diagnostics);

  private:
    Spec parse_spec(DiagnosticBag& diagnostics);
    IncludeDecl parse_include_decl(DiagnosticBag& diagnostics);
    SystemDecl parse_system_decl(DiagnosticBag& diagnostics);
    NamespaceDecl parse_namespace_decl(
        DiagnosticBag& diagnostics,
        SystemDecl& system,
        const std::string& prefix = ""
    );
    ValueDecl parse_value_decl(DiagnosticBag& diagnostics);
    EnumDecl parse_enum_decl(DiagnosticBag& diagnostics);
    EventDecl parse_event_decl(DiagnosticBag& diagnostics);
    ExternalSystemDecl parse_external_system_decl(DiagnosticBag& diagnostics);
    ShapeDecl parse_shape_decl(DiagnosticBag& diagnostics);
    FeatureFlagDecl parse_feature_flag_decl(DiagnosticBag& diagnostics);
    LogDecl parse_log_decl(DiagnosticBag& diagnostics);
    MetricDecl parse_metric_decl(DiagnosticBag& diagnostics);
    EntityDecl parse_entity_decl(DiagnosticBag& diagnostics);
    QueueDecl parse_queue_decl(DiagnosticBag& diagnostics);
    MessageDecl parse_message_decl(DiagnosticBag& diagnostics);
    LeaseDecl parse_lease_decl(DiagnosticBag& diagnostics);
    WorkerDecl parse_worker_decl(DiagnosticBag& diagnostics);
    ApiServerDecl parse_api_server_decl(DiagnosticBag& diagnostics);
    ApiDecl parse_api_decl(DiagnosticBag& diagnostics);
    WorkflowDecl parse_workflow_decl(DiagnosticBag& diagnostics);
    WorkflowLoadDecl parse_workflow_load_decl(DiagnosticBag& diagnostics);
    WorkflowStepDecl parse_workflow_step_decl(DiagnosticBag& diagnostics);
    WorkflowStatementDecl parse_workflow_statement_decl(DiagnosticBag& diagnostics);
    std::vector<WorkflowAssignmentDecl> parse_workflow_payload(DiagnosticBag& diagnostics);
    PolicyDecl parse_policy_decl(DiagnosticBag& diagnostics);
    StateMachineDecl parse_state_machine_decl(DiagnosticBag& diagnostics);
    StateDecl parse_state_decl(DiagnosticBag& diagnostics);
    GarbageCollectionPolicyDecl parse_garbage_collection_policy_decl(DiagnosticBag& diagnostics);
    OwnershipDecl parse_ownership_decl(DiagnosticBag& diagnostics);
    RelationDecl parse_relation_decl(DiagnosticBag& diagnostics);
    ChildDecl parse_child_decl(DiagnosticBag& diagnostics);
    InvariantDecl parse_invariant_decl(DiagnosticBag& diagnostics);
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
    std::string parse_expression_until(TokenKind kind);

    void consume_optional_semicolon();
};

} // namespace statespec
