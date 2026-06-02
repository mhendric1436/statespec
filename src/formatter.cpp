#include "statespec/formatter.hpp"

#include "statespec/language_constants.hpp"

#include <algorithm>
#include <sstream>
#include <string_view>

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
    case TokenKind::KeywordChildWorkflow:
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
           (token.lexeme == SyntaxIdentifierType || token.lexeme == SyntaxIdentifierDefault ||
            token.lexeme == SyntaxIdentifierScope || token.lexeme == SyntaxIdentifierOwner ||
            token.lexeme == SyntaxIdentifierDescription || token.lexeme == SyntaxIdentifierExpires);
}

bool is_state_option_member_start(const Token& token)
{
    return token.kind == TokenKind::KeywordTerminal ||
           (token.kind == TokenKind::Identifier &&
            token.lexeme == SyntaxIdentifierGarbageCollection);
}

bool is_garbage_collection_member_start(const Token& token)
{
    return token.kind == TokenKind::Identifier &&
           (token.lexeme == SyntaxIdentifierAfter || token.lexeme == SyntaxIdentifierMode);
}

class AstFormatter
{
  public:
    std::string format(const Spec& spec)
    {
        if (spec.version.has_value())
        {
            line("statespec " + *spec.version + ";");
        }
        for (const auto& include : spec.includes)
        {
            line("include " + quoted(include.path) + ";");
        }
        if (spec.system.has_value())
        {
            write_system(*spec.system);
        }
        auto result = out_.str();
        while (!result.empty() && result.back() == '\n')
        {
            result.pop_back();
        }
        result.push_back('\n');
        return result;
    }

  private:
    std::ostringstream out_;
    int indent_ = 0;
    bool previous_blank_ = false;

    void write_indent()
    {
        for (int i = 0; i < indent_; ++i)
        {
            out_ << "  ";
        }
    }

    void line(std::string_view text = {})
    {
        write_indent();
        out_ << text << '\n';
        previous_blank_ = false;
    }

    void blank()
    {
        if (!previous_blank_)
        {
            out_ << '\n';
            previous_blank_ = true;
        }
    }

    template <typename Fn>
    void block(
        std::string_view header,
        Fn&& body
    )
    {
        line(std::string{header} + " {");
        ++indent_;
        body();
        --indent_;
        line("}");
    }

    static std::string join(
        const std::vector<std::string>& values,
        std::string_view separator
    )
    {
        std::ostringstream joined;
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            if (i > 0)
            {
                joined << separator;
            }
            joined << values[i];
        }
        return joined.str();
    }

    static std::string bool_text(bool value)
    {
        return value ? std::string{SyntaxBooleanTrue} : std::string{"false"};
    }

    static std::string quoted(std::string_view value)
    {
        std::ostringstream text;
        text << '"';
        for (const char ch : value)
        {
            if (ch == '"' || ch == '\\')
            {
                text << '\\';
            }
            text << ch;
        }
        text << '"';
        return text.str();
    }

    static std::string literal_text(
        const std::string& value,
        const std::optional<std::string>& kind
    )
    {
        return kind == token_kind_name(TokenKind::StringLiteral) ? quoted(value) : value;
    }

    static std::string normalized_expression(std::string expression)
    {
        auto replace_all = [&](std::string_view from, std::string_view to)
        {
            std::size_t pos = 0;
            while ((pos = expression.find(from, pos)) != std::string::npos)
            {
                expression.replace(pos, from.size(), to);
                pos += to.size();
            }
        };
        replace_all(" . ", ".");
        replace_all("[ ", "[");
        replace_all(" ]", "]");
        replace_all("( ", "(");
        replace_all(" )", ")");
        replace_all(" ,", ",");
        return expression;
    }

    static std::string bracketed(const std::vector<std::string>& values)
    {
        return "[" + join(values, ", ") + "]";
    }

    void write_fields_block(
        std::string_view name,
        const std::vector<FieldDecl>& fields
    )
    {
        block(
            name,
            [&]()
            {
                for (const auto& field : fields)
                {
                    line(field.name + " " + field.type);
                }
            }
        );
    }

    void write_system(const SystemDecl& system)
    {
        block(
            "system " + system.name,
            [&]()
            {
                bool wrote = false;
                auto section = [&]()
                {
                    if (wrote)
                    {
                        blank();
                    }
                    wrote = true;
                };

                if (system.tenant_scope.has_value())
                {
                    section();
                    line("tenant scoped_by " + system.tenant_scope->field_name);
                }
                if (system.system_tenant.has_value())
                {
                    if (!wrote)
                    {
                        section();
                    }
                    line("system_tenant configured");
                }

                for (const auto& feature_flag : system.feature_flags)
                {
                    section();
                    write_feature_flag(feature_flag);
                }
                for (const auto& log : system.logs)
                {
                    section();
                    write_log(log);
                }
                for (const auto& metric : system.metrics)
                {
                    section();
                    write_metric(metric);
                }
                for (const auto& value : system.values)
                {
                    section();
                    write_value(value);
                }
                for (const auto& enum_decl : system.enums)
                {
                    section();
                    write_enum(enum_decl);
                }
                for (const auto& event : system.events)
                {
                    section();
                    write_event(event);
                }
                for (const auto& external_system : system.external_systems)
                {
                    section();
                    write_external_system(external_system);
                }
                for (const auto& shape : system.shapes)
                {
                    section();
                    write_shape(shape);
                }
                for (const auto& entity : system.entities)
                {
                    section();
                    write_entity(entity);
                }
                for (const auto& queue : system.queues)
                {
                    section();
                    write_queue(queue);
                }
                for (const auto& lease : system.leases)
                {
                    section();
                    write_lease(lease);
                }
                for (const auto& worker : system.workers)
                {
                    section();
                    write_worker(worker);
                }
                for (const auto& api : system.apis)
                {
                    section();
                    write_api(api);
                }
                for (const auto& api_server : system.api_servers)
                {
                    section();
                    write_api_server(api_server);
                }
                for (const auto& workflow : system.workflows)
                {
                    section();
                    write_workflow(workflow);
                }
                for (const auto& policy : system.policies)
                {
                    section();
                    write_policy(policy);
                }
            }
        );
    }

    void write_value(const ValueDecl& value)
    {
        std::string text = "value " + value.name + ": " + value.type;
        if (value.constraint.has_value())
        {
            text += " where " + normalized_expression(*value.constraint);
        }
        line(text + ";");
    }

    void write_enum(const EnumDecl& enum_decl)
    {
        block(
            "enum " + enum_decl.name,
            [&]()
            {
                for (const auto& member : enum_decl.members)
                {
                    if (member.value.has_value())
                    {
                        line(member.name + " = " + literal_text(*member.value, member.value_kind));
                    }
                    else
                    {
                        line(member.name);
                    }
                }
            }
        );
    }

    void write_shape(const ShapeDecl& shape)
    {
        write_fields_block("shape " + shape.name, shape.fields);
    }

    void write_event(const EventDecl& event)
    {
        block("event " + event.name, [&]() { write_fields_block("fields", event.fields); });
    }

    void write_feature_flag(const FeatureFlagDecl& feature_flag)
    {
        block(
            "feature_flag " + feature_flag.name,
            [&]()
            {
                if (feature_flag.type.has_value())
                {
                    line("type " + *feature_flag.type);
                }
                if (feature_flag.default_value.has_value())
                {
                    line(
                        "default " +
                        literal_text(*feature_flag.default_value, feature_flag.default_value_kind)
                    );
                }
                if (feature_flag.scope.has_value())
                {
                    line("scope " + *feature_flag.scope);
                }
                if (feature_flag.owner.has_value())
                {
                    line("owner " + quoted(*feature_flag.owner));
                }
                if (feature_flag.description.has_value())
                {
                    line("description " + quoted(*feature_flag.description));
                }
                if (feature_flag.expires.has_value())
                {
                    line("expires " + quoted(*feature_flag.expires));
                }
            }
        );
    }

    void write_log(const LogDecl& log)
    {
        block(
            "log " + log.name,
            [&]()
            {
                if (log.level.has_value())
                {
                    line("level " + *log.level);
                }
                if (log.event_name.has_value())
                {
                    line("event_name " + quoted(*log.event_name));
                }
                if (!log.fields.empty())
                {
                    write_fields_block("fields", log.fields);
                }
            }
        );
    }

    void write_metric(const MetricDecl& metric)
    {
        block(
            "metric " + metric.name,
            [&]()
            {
                if (metric.kind.has_value())
                {
                    line("kind " + *metric.kind);
                }
                if (metric.backend_name.has_value())
                {
                    line("name " + quoted(*metric.backend_name));
                }
                if (metric.unit.has_value())
                {
                    line("unit " + *metric.unit);
                }
                if (!metric.labels.empty())
                {
                    write_fields_block("labels", metric.labels);
                }
            }
        );
    }

    void write_external_system(const ExternalSystemDecl& external_system)
    {
        block(
            "external_system " + external_system.name,
            [&]()
            {
                for (const auto& property : external_system.properties)
                {
                    line(property.name + ": " + literal_text(property.value, property.value_kind));
                }
                if (external_system.metadata.has_value())
                {
                    write_external_system_metadata(*external_system.metadata);
                }
            }
        );
    }

    void write_external_system_metadata(const ExternalSystemMetadataDecl& metadata)
    {
        block(
            "metadata",
            [&]()
            {
                if (metadata.entity.has_value())
                {
                    line("entity " + *metadata.entity);
                }
                if (metadata.profile_field.has_value())
                {
                    line("profile_field " + *metadata.profile_field);
                }
                if (!metadata.required_fields.empty())
                {
                    line("required_fields" + bracketed(metadata.required_fields));
                }
                if (!metadata.mappings.empty())
                {
                    block(
                        "mappings",
                        [&]()
                        {
                            for (const auto& mapping : metadata.mappings)
                            {
                                line(mapping.source + " -> " + mapping.target);
                            }
                        }
                    );
                }
            }
        );
    }

    void write_entity(const EntityDecl& entity)
    {
        block(
            "entity " + entity.name,
            [&]()
            {
                if (!entity.key_fields.empty())
                {
                    line("key " + join(entity.key_fields, ", "));
                }
                if (entity.ownership.has_value())
                {
                    write_ownership(*entity.ownership);
                }
                write_fields_block("fields", entity.fields);
                if (entity.state_machine.has_value())
                {
                    write_state_machine(*entity.state_machine);
                }
                if (!entity.relations.empty())
                {
                    write_relations(entity.relations);
                }
                if (!entity.children.empty())
                {
                    write_children(entity.children);
                }
                if (!entity.invariants.empty())
                {
                    write_invariants(entity.invariants);
                }
                if (!entity.indexes.empty())
                {
                    write_indexes(entity.indexes);
                }
                if (entity.api.has_value())
                {
                    write_entity_api(*entity.api);
                }
            }
        );
    }

    void write_ownership(const OwnershipDecl& ownership)
    {
        block(
            "ownership",
            [&]()
            {
                if (ownership.authority.has_value())
                {
                    line("authority: " + *ownership.authority);
                }
                if (ownership.system_of_record.has_value())
                {
                    line("system_of_record: " + *ownership.system_of_record);
                }
                if (ownership.lifecycle.has_value())
                {
                    line("lifecycle: " + *ownership.lifecycle);
                }
            }
        );
    }

    void write_relations(const std::vector<RelationDecl>& relations)
    {
        block(
            "relations",
            [&]()
            {
                for (const auto& relation : relations)
                {
                    std::string target = relation.target;
                    if (relation.kind == SyntaxKeywordParent && relation.optional &&
                        target.rfind("optional ", 0) != 0 && target.find('?') == std::string::npos)
                    {
                        target = "optional " + target;
                    }
                    std::string header = relation.kind + " " + relation.name + ": " + target;
                    const bool has_options = relation.relation_kind.has_value() ||
                                             relation.on_parent_delete.has_value() ||
                                             !relation.parent_must_be_in.empty() ||
                                             !relation.unique_within_parent.empty();
                    if (!has_options)
                    {
                        line(header);
                        continue;
                    }
                    block(
                        header,
                        [&]()
                        {
                            if (relation.relation_kind.has_value())
                            {
                                line("kind: " + *relation.relation_kind);
                            }
                            if (relation.on_parent_delete.has_value())
                            {
                                line("on_parent_delete: " + *relation.on_parent_delete);
                            }
                            if (!relation.parent_must_be_in.empty())
                            {
                                line("parent_must_be_in: " + bracketed(relation.parent_must_be_in));
                            }
                            if (!relation.unique_within_parent.empty())
                            {
                                line(
                                    "unique_within_parent: " +
                                    bracketed(relation.unique_within_parent)
                                );
                            }
                        }
                    );
                }
            }
        );
    }

    void write_children(const std::vector<ChildDecl>& children)
    {
        block(
            "children",
            [&]()
            {
                for (const auto& child : children)
                {
                    line(child.name + ": " + child.target_entity + " by " + child.relation);
                }
            }
        );
    }

    void write_invariants(const std::vector<InvariantDecl>& invariants)
    {
        block(
            "invariants",
            [&]()
            {
                for (const auto& invariant : invariants)
                {
                    line(invariant.name + ": " + normalized_expression(invariant.expression));
                }
            }
        );
    }

    void write_indexes(const std::vector<IndexDecl>& indexes)
    {
        block(
            "indexes",
            [&]()
            {
                for (const auto& index : indexes)
                {
                    line(
                        std::string{index.unique ? "unique " : "index "} + index.name + " on " +
                        join(index.fields, ", ")
                    );
                }
            }
        );
    }

    void write_entity_api(const EntityApiDecl& api)
    {
        block(
            "api",
            [&]()
            {
                if (api.resource.has_value())
                {
                    line("resource " + quoted(*api.resource));
                }
                if (api.create.has_value())
                {
                    const auto& create = *api.create;
                    std::string header = "create";
                    if (create.name.has_value())
                    {
                        header += " " + *create.name;
                    }
                    if (create.fields.empty())
                    {
                        line(header);
                    }
                    else
                    {
                        block(header, [&]() { line("fields " + bracketed(create.fields)); });
                    }
                }
                if (api.get.has_value())
                {
                    write_entity_api_simple_operation("get", *api.get);
                }
                for (const auto& list : api.lists)
                {
                    std::string header = "list";
                    if (list.name.has_value())
                    {
                        header += " " + *list.name;
                    }
                    block(
                        header,
                        [&]()
                        {
                            if (list.path.has_value())
                            {
                                line("path " + quoted(*list.path));
                            }
                            if (!list.by.empty())
                            {
                                line("by " + bracketed(list.by));
                            }
                        }
                    );
                }
                if (api.update_status.has_value())
                {
                    write_entity_api_simple_operation("update_status", *api.update_status);
                }
                if (api.delete_.has_value())
                {
                    write_entity_api_simple_operation("delete", *api.delete_);
                }
            }
        );
    }

    void write_entity_api_simple_operation(
        std::string_view keyword,
        const EntityApiOperationDecl& operation
    )
    {
        std::string text{keyword};
        if (operation.name.has_value())
        {
            text += " " + *operation.name;
        }
        line(text);
    }

    void write_state_machine(const StateMachineDecl& state_machine)
    {
        block(
            "state_machine",
            [&]()
            {
                for (const auto& state : state_machine.states)
                {
                    if (state.terminal || state.garbage_collection.has_value())
                    {
                        block(
                            "state " + state.name,
                            [&]()
                            {
                                if (state.terminal)
                                {
                                    line("terminal: true");
                                }
                                if (state.garbage_collection.has_value())
                                {
                                    block(
                                        "garbage_collection",
                                        [&]()
                                        {
                                            if (state.garbage_collection->after.has_value())
                                            {
                                                line("after: " + *state.garbage_collection->after);
                                            }
                                            if (state.garbage_collection->mode.has_value())
                                            {
                                                line("mode: " + *state.garbage_collection->mode);
                                            }
                                        }
                                    );
                                }
                            }
                        );
                    }
                    else
                    {
                        line("state " + state.name);
                    }
                }
                if (state_machine.initial_state.has_value())
                {
                    line("initial " + *state_machine.initial_state);
                }
                if (!state_machine.terminal_states.empty())
                {
                    if (state_machine.terminal_states.size() == 1)
                    {
                        line("terminal " + state_machine.terminal_states.front());
                    }
                    else
                    {
                        line("terminal" + bracketed(state_machine.terminal_states));
                    }
                }
                for (const auto& transition : state_machine.transitions)
                {
                    line(transition.from + " -> " + transition.to);
                }
            }
        );
    }

    void write_queue(const QueueDecl& queue)
    {
        block(
            "queue " + queue.name,
            [&]()
            {
                if (queue.namespace_name.has_value())
                {
                    line("namespace " + *queue.namespace_name);
                }
                if (queue.channel.has_value())
                {
                    line("channel " + *queue.channel);
                }
                if (queue.visibility_timeout.has_value())
                {
                    line("visibility_timeout " + *queue.visibility_timeout);
                }
                if (queue.max_attempts.has_value())
                {
                    line("max_attempts " + std::to_string(*queue.max_attempts));
                }
                if (queue.dead_letter.has_value())
                {
                    line("dead_letter " + *queue.dead_letter);
                }
                for (const auto& message : queue.messages)
                {
                    block(
                        "message " + message.name,
                        [&]()
                        {
                            if (message.idempotency_key.has_value())
                            {
                                line("idempotency_key " + *message.idempotency_key);
                            }
                            write_fields_block("payload", message.payload_fields);
                        }
                    );
                }
            }
        );
    }

    void write_lease(const LeaseDecl& lease)
    {
        block(
            "lease " + lease.name,
            [&]()
            {
                if (lease.resource.has_value())
                {
                    line("resource " + quoted(*lease.resource));
                }
                if (lease.ttl.has_value())
                {
                    line("ttl " + *lease.ttl);
                }
                if (lease.renew_every.has_value())
                {
                    line("renew_every " + *lease.renew_every);
                }
                if (lease.holder.has_value())
                {
                    line("holder " + *lease.holder);
                }
                if (lease.fencing_token.has_value())
                {
                    line("fencing_token " + bool_text(*lease.fencing_token));
                }
                if (lease.max_ttl.has_value())
                {
                    line("max_ttl " + *lease.max_ttl);
                }
            }
        );
    }

    void write_worker(const WorkerDecl& worker)
    {
        block(
            "worker " + worker.name,
            [&]()
            {
                if (worker.singleton.has_value())
                {
                    line("singleton " + bool_text(*worker.singleton));
                }
                if (worker.lease.has_value())
                {
                    line("lease " + *worker.lease);
                }
                if (worker.polls.has_value())
                {
                    line("polls " + *worker.polls);
                }
                if (worker.executes.has_value())
                {
                    line("executes " + *worker.executes);
                }
                if (worker.concurrency.has_value())
                {
                    line("concurrency " + std::to_string(*worker.concurrency));
                }
            }
        );
    }

    void write_api(const ApiDecl& api)
    {
        block(
            "api " + api.name,
            [&]()
            {
                if (api.method.has_value())
                {
                    line("method " + *api.method);
                }
                if (api.path.has_value())
                {
                    line("path " + quoted(*api.path));
                }
                if (api.input.has_value())
                {
                    line("input " + *api.input);
                }
                if (api.output.has_value())
                {
                    line("output " + *api.output);
                }
                if (api.error.has_value())
                {
                    line("error " + *api.error);
                }
                if (api.starts_workflow.has_value())
                {
                    line("starts workflow " + *api.starts_workflow);
                }
                if (api.enqueues.has_value())
                {
                    line("enqueues " + *api.enqueues);
                }
            }
        );
    }

    void write_api_server(const ApiServerDecl& api_server)
    {
        block(
            "api_server " + api_server.name,
            [&]()
            {
                for (const auto& served : api_server.serves)
                {
                    line("serves " + served);
                }
                if (api_server.concurrency.has_value())
                {
                    line("concurrency " + std::to_string(*api_server.concurrency));
                }
            }
        );
    }

    void write_workflow(const WorkflowDecl& workflow)
    {
        block(
            "workflow " + workflow.name,
            [&]()
            {
                if (workflow.version.has_value())
                {
                    line("version " + std::to_string(*workflow.version));
                }
                if (workflow.singleton.has_value())
                {
                    line("singleton " + bool_text(*workflow.singleton));
                }
                if (workflow.expected_execution_time.has_value())
                {
                    line("expected_execution_time " + *workflow.expected_execution_time);
                }
                if (workflow.start_step.has_value())
                {
                    line("start " + *workflow.start_step);
                }
                if (workflow.on.has_value())
                {
                    line("on " + *workflow.on);
                }
                if (workflow.input.has_value())
                {
                    line("input " + *workflow.input);
                }
                if (workflow.state.has_value())
                {
                    line("state " + *workflow.state);
                }
                for (const auto& load : workflow.loads)
                {
                    line("load " + load.entity + " by " + load.key_field + " as " + load.binding);
                }
                for (const auto& child_set : workflow.child_sets)
                {
                    write_workflow_child_set(child_set);
                }
                for (const auto& child_workflow : workflow.child_workflows)
                {
                    write_workflow_child_workflow(child_workflow);
                }
                for (const auto& step : workflow.steps)
                {
                    write_workflow_step(step);
                }
            }
        );
    }

    void write_workflow_child_set(const WorkflowChildSetDecl& child_set)
    {
        block(
            "child_set " + child_set.name,
            [&]()
            {
                if (child_set.entity.has_value())
                {
                    line("entity " + *child_set.entity);
                }
                if (child_set.parent_field.has_value())
                {
                    line("parent_field " + *child_set.parent_field);
                }
                if (child_set.id_type.has_value())
                {
                    line("id_type " + *child_set.id_type);
                }
                if (child_set.pending.has_value())
                {
                    line("pending " + *child_set.pending);
                }
                if (child_set.creating.has_value())
                {
                    line("creating " + *child_set.creating);
                }
                if (child_set.succeeded.has_value())
                {
                    line("succeeded " + *child_set.succeeded);
                }
                if (child_set.failed.has_value())
                {
                    line("failed " + *child_set.failed);
                }
                if (child_set.desired_count.has_value())
                {
                    line("desired_count " + normalized_expression(*child_set.desired_count));
                }
            }
        );
    }

    void write_workflow_child_workflow(const WorkflowChildWorkflowDecl& child_workflow)
    {
        block(
            "child_workflow " + child_workflow.name,
            [&]()
            {
                if (child_workflow.child_entity.has_value())
                {
                    line("child_entity " + *child_workflow.child_entity);
                }
                if (child_workflow.child_workflow.has_value())
                {
                    line("child_workflow " + *child_workflow.child_workflow);
                }
                if (child_workflow.child_id_field.has_value() &&
                    child_workflow.child_id_type.has_value())
                {
                    line(
                        "child_id " + *child_workflow.child_id_field + " " +
                        *child_workflow.child_id_type
                    );
                }
                if (child_workflow.parent_ref_field.has_value() &&
                    child_workflow.parent_ref_expression.has_value())
                {
                    line(
                        "parent_ref " + *child_workflow.parent_ref_field + " = " +
                        normalized_expression(*child_workflow.parent_ref_expression)
                    );
                }
                if (child_workflow.desired_count.has_value())
                {
                    line("desired_count " + normalized_expression(*child_workflow.desired_count));
                }
                if (!child_workflow.create_assignments.empty())
                {
                    block(
                        "create",
                        [&]()
                        {
                            for (const auto& assignment : child_workflow.create_assignments)
                            {
                                line(
                                    assignment.name + ": " +
                                    normalized_expression(assignment.expression)
                                );
                            }
                        }
                    );
                }
                if (child_workflow.success_expression.has_value())
                {
                    line(
                        "success when " + normalized_expression(*child_workflow.success_expression)
                    );
                }
                if (child_workflow.failure_expression.has_value())
                {
                    line(
                        "failure when " + normalized_expression(*child_workflow.failure_expression)
                    );
                }
            }
        );
    }

    void write_workflow_step(const WorkflowStepDecl& step)
    {
        block(
            "step " + step.name,
            [&]()
            {
                if (step.expected_execution_time.has_value())
                {
                    line("expected_execution_time " + *step.expected_execution_time);
                }
                if (step.max_retries.has_value())
                {
                    line("max_retries " + std::to_string(*step.max_retries));
                }
                for (const auto& statement : step.statements)
                {
                    write_workflow_statement(statement);
                }
            }
        );
    }

    void write_workflow_payload(const std::vector<WorkflowAssignmentDecl>& payload)
    {
        for (const auto& assignment : payload)
        {
            line(assignment.name + " = " + normalized_expression(assignment.expression) + ";");
        }
    }

    void write_workflow_statement(const WorkflowStatementDecl& statement)
    {
        if (statement.kind == "require" && statement.expression.has_value())
        {
            line("require " + normalized_expression(*statement.expression) + ";");
        }
        else if (statement.kind == "set" && statement.assignable.has_value() &&
                 statement.expression.has_value())
        {
            line(
                "set " + normalized_expression(*statement.assignable) + " = " +
                normalized_expression(*statement.expression) + ";"
            );
        }
        else if ((statement.kind == "emit" || statement.kind == "enqueue") &&
                 statement.target.has_value())
        {
            if (statement.payload.empty())
            {
                line(statement.kind + " " + *statement.target + ";");
            }
            else
            {
                block(
                    statement.kind + " " + *statement.target,
                    [&]() { write_workflow_payload(statement.payload); }
                );
            }
        }
        else if (statement.kind == "acquire_lease" && statement.target.has_value())
        {
            std::string text = "acquire lease " + *statement.target;
            if (statement.binding.has_value())
            {
                text += " as " + *statement.binding;
            }
            line(text + ";");
        }
        else if (statement.kind == "renew_lease" && statement.target.has_value())
        {
            line("renew lease " + *statement.target + ";");
        }
        else if (statement.kind == "release_lease" && statement.target.has_value())
        {
            line("release lease " + *statement.target + ";");
        }
        else if (statement.kind == "start_workflow" && statement.target.has_value())
        {
            if (statement.payload.empty())
            {
                line("start workflow " + *statement.target + ";");
            }
            else
            {
                block(
                    "start workflow " + *statement.target,
                    [&]() { write_workflow_payload(statement.payload); }
                );
            }
        }
        else if (statement.kind == "transition_to" && statement.target.has_value())
        {
            line("transition_to " + *statement.target + ";");
        }
        else if (statement.kind == "complete")
        {
            line("complete;");
        }
        else if (statement.kind == "fail")
        {
            line(
                statement.expression.has_value()
                    ? "fail " + normalized_expression(*statement.expression) + ";"
                    : "fail;"
            );
        }
        else if (statement.kind == "atomic")
        {
            block("atomic", [&]() { write_workflow_statements(statement.statements); });
        }
        else if (statement.kind == "for_each" && statement.binding.has_value() &&
                 statement.expression.has_value())
        {
            block(
                "for_each " + *statement.binding + " in " +
                    normalized_expression(*statement.expression),
                [&]() { write_workflow_statements(statement.statements); }
            );
        }
        else if (statement.kind == "when" && statement.expression.has_value())
        {
            block(
                "when " + normalized_expression(*statement.expression),
                [&]() { write_workflow_statements(statement.statements); }
            );
        }
        else if (statement.kind == "create_child" && statement.target.has_value())
        {
            block(
                "create child " + *statement.target,
                [&]() { write_workflow_payload(statement.payload); }
            );
        }
        else if (statement.kind == "observe_child" && statement.target.has_value() &&
                 statement.binding.has_value() && statement.expression.has_value())
        {
            line(
                "observe child " + *statement.target + " by " + *statement.binding + " = " +
                normalized_expression(*statement.expression) + ";"
            );
        }
        else if (statement.kind == "move" && statement.expression.has_value() &&
                 statement.from_assignable.has_value() && statement.to_assignable.has_value())
        {
            line(
                "move " + normalized_expression(*statement.expression) + " from " +
                normalized_expression(*statement.from_assignable) + " to " +
                normalized_expression(*statement.to_assignable) + ";"
            );
        }
        else if (statement.kind == "reserve_child_set" && statement.target.has_value())
        {
            line("reserve child_set " + *statement.target + ";");
        }
        else if (statement.kind == "materialize_child_set" && statement.target.has_value())
        {
            line("materialize child_set " + *statement.target + ";");
        }
        else if (statement.kind == "reconcile_child_set" && statement.target.has_value())
        {
            line("reconcile child_set " + *statement.target + ";");
        }
    }

    void write_workflow_statements(const std::vector<WorkflowStatementDecl>& statements)
    {
        for (const auto& statement : statements)
        {
            write_workflow_statement(statement);
        }
    }

    void write_policy(const PolicyDecl& policy)
    {
        block(
            "policy " + policy.name,
            [&]()
            {
                if (policy.tenant_scoped_by.has_value())
                {
                    line("tenant scoped_by " + *policy.tenant_scoped_by);
                }
                for (const auto& allow : policy.allows)
                {
                    line(
                        "allow " + allow.action + " when " +
                        normalized_expression(allow.condition) + ";"
                    );
                }
                for (const auto& deny : policy.denies)
                {
                    line(
                        "deny " + deny.action + " when " + normalized_expression(deny.condition) +
                        ";"
                    );
                }
                for (const auto& quota : policy.quotas)
                {
                    line(
                        "quota " + quota.name + ": " + normalized_expression(quota.expression) + ";"
                    );
                }
                for (const auto& audit : policy.audits)
                {
                    line("audit " + audit + ";");
                }
            }
        );
    }
};

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
            !block_name_stack.empty() &&
            block_name_stack.back() == SyntaxIdentifierGarbageCollection;
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

std::string format_spec_ast(
    const Spec& spec,
    DiagnosticBag& diagnostics
)
{
    if (diagnostics.has_errors())
    {
        return "";
    }
    return AstFormatter{}.format(spec);
}

} // namespace statespec
