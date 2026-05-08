#include "statespec/diagnostic.hpp"
#include "statespec/lexer.hpp"
#include "statespec/parser.hpp"
#include "statespec/source.hpp"
#include "statespec/token.hpp"
#include "statespec/validator.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

std::string read_file(const std::string& path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("failed to open file: " + path);
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void print_diagnostics(const statespec::DiagnosticBag& diagnostics)
{
    for (const auto& diagnostic : diagnostics.all())
    {
        const char* severity = "error";
        if (diagnostic.severity == statespec::DiagnosticSeverity::Warning)
        {
            severity = "warning";
        }
        else if (diagnostic.severity == statespec::DiagnosticSeverity::Info)
        {
            severity = "info";
        }

        std::cerr << diagnostic.range.begin.line << ':' << diagnostic.range.begin.column << ": "
                  << severity << ' ' << diagnostic.code << ": " << diagnostic.message << '\n';
    }
}

std::vector<statespec::Token> lex_file(
    const std::string& path,
    statespec::DiagnosticBag& diagnostics
)
{
    statespec::SourceFile source{path, read_file(path)};
    statespec::Lexer lexer{source};
    return lexer.lex(diagnostics);
}

std::string json_escape(const std::string& value)
{
    std::ostringstream out;
    for (const auto ch : value)
    {
        switch (ch)
        {
        case '"':
            out << "\\\"";
            break;
        case '\\':
            out << "\\\\";
            break;
        case '\b':
            out << "\\b";
            break;
        case '\f':
            out << "\\f";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            out << ch;
            break;
        }
    }
    return out.str();
}

void write_indent(
    std::ostream& out,
    int indent
)
{
    for (int i = 0; i < indent; ++i)
    {
        out << ' ';
    }
}

void write_json_string(
    std::ostream& out,
    const std::string& value
)
{
    out << '"' << json_escape(value) << '"';
}

void write_json_optional_string(
    std::ostream& out,
    const std::optional<std::string>& value
)
{
    if (value.has_value())
    {
        write_json_string(out, *value);
    }
    else
    {
        out << "null";
    }
}

void write_json_optional_int(
    std::ostream& out,
    const std::optional<int>& value
)
{
    if (value.has_value())
    {
        out << *value;
    }
    else
    {
        out << "null";
    }
}

void write_json_optional_bool(
    std::ostream& out,
    const std::optional<bool>& value
)
{
    if (value.has_value())
    {
        out << (*value ? "true" : "false");
    }
    else
    {
        out << "null";
    }
}

void write_json_string_array(
    std::ostream& out,
    const std::vector<std::string>& values,
    int indent
)
{
    out << "[";
    if (!values.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        write_indent(out, indent + 2);
        write_json_string(out, values[i]);
        if (i + 1 < values.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!values.empty())
    {
        write_indent(out, indent);
    }
    out << "]";
}

void write_fields(
    std::ostream& out,
    const std::vector<statespec::FieldDecl>& fields,
    int indent
)
{
    out << "[";
    if (!fields.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < fields.size(); ++i)
    {
        const auto& field = fields[i];
        write_indent(out, indent + 2);
        out << "{\"name\": ";
        write_json_string(out, field.name);
        out << ", \"type\": ";
        write_json_string(out, field.type);
        out << "}";
        if (i + 1 < fields.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!fields.empty())
    {
        write_indent(out, indent);
    }
    out << "]";
}

void write_state_machine(
    std::ostream& out,
    const statespec::StateMachineDecl& state_machine,
    int indent
)
{
    out << "{\n";

    write_indent(out, indent + 2);
    out << "\"states\": [";
    if (!state_machine.states.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < state_machine.states.size(); ++i)
    {
        write_indent(out, indent + 4);
        out << "{\"name\": ";
        write_json_string(out, state_machine.states[i].name);
        out << ", \"terminal\": " << (state_machine.states[i].terminal ? "true" : "false") << "}";
        if (i + 1 < state_machine.states.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!state_machine.states.empty())
    {
        write_indent(out, indent + 2);
    }
    out << "],\n";

    write_indent(out, indent + 2);
    out << "\"initial_state\": ";
    write_json_optional_string(out, state_machine.initial_state);
    out << ",\n";

    write_indent(out, indent + 2);
    out << "\"terminal_states\": ";
    write_json_string_array(out, state_machine.terminal_states, indent + 2);
    out << ",\n";

    write_indent(out, indent + 2);
    out << "\"transitions\": [";
    if (!state_machine.transitions.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < state_machine.transitions.size(); ++i)
    {
        const auto& transition = state_machine.transitions[i];
        write_indent(out, indent + 4);
        out << "{\"from\": ";
        write_json_string(out, transition.from);
        out << ", \"to\": ";
        write_json_string(out, transition.to);
        out << "}";
        if (i + 1 < state_machine.transitions.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!state_machine.transitions.empty())
    {
        write_indent(out, indent + 2);
    }
    out << "]\n";

    write_indent(out, indent);
    out << "}";
}

void write_entities(
    std::ostream& out,
    const std::vector<statespec::EntityDecl>& entities,
    int indent
)
{
    out << "[";
    if (!entities.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < entities.size(); ++i)
    {
        const auto& entity = entities[i];
        write_indent(out, indent + 2);
        out << "{\n";
        write_indent(out, indent + 4);
        out << "\"name\": ";
        write_json_string(out, entity.name);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"key_fields\": ";
        write_json_string_array(out, entity.key_fields, indent + 4);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"fields\": ";
        write_fields(out, entity.fields, indent + 4);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"state_machine\": ";
        if (entity.state_machine.has_value())
        {
            write_state_machine(out, *entity.state_machine, indent + 4);
        }
        else
        {
            out << "null";
        }
        out << '\n';
        write_indent(out, indent + 2);
        out << "}";
        if (i + 1 < entities.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!entities.empty())
    {
        write_indent(out, indent);
    }
    out << "]";
}

void write_queues(
    std::ostream& out,
    const std::vector<statespec::QueueDecl>& queues,
    int indent
)
{
    out << "[";
    if (!queues.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < queues.size(); ++i)
    {
        const auto& queue = queues[i];
        write_indent(out, indent + 2);
        out << "{\n";
        write_indent(out, indent + 4);
        out << "\"name\": ";
        write_json_string(out, queue.name);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"namespace\": ";
        write_json_optional_string(out, queue.namespace_name);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"channel\": ";
        write_json_optional_string(out, queue.channel);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"visibility_timeout\": ";
        write_json_optional_string(out, queue.visibility_timeout);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"max_attempts\": ";
        write_json_optional_int(out, queue.max_attempts);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"dead_letter\": ";
        write_json_optional_string(out, queue.dead_letter);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"messages\": [";
        if (!queue.messages.empty())
        {
            out << '\n';
        }
        for (std::size_t j = 0; j < queue.messages.size(); ++j)
        {
            const auto& message = queue.messages[j];
            write_indent(out, indent + 6);
            out << "{\n";
            write_indent(out, indent + 8);
            out << "\"name\": ";
            write_json_string(out, message.name);
            out << ",\n";
            write_indent(out, indent + 8);
            out << "\"idempotency_key\": ";
            write_json_optional_string(out, message.idempotency_key);
            out << ",\n";
            write_indent(out, indent + 8);
            out << "\"payload_fields\": ";
            write_fields(out, message.payload_fields, indent + 8);
            out << '\n';
            write_indent(out, indent + 6);
            out << "}";
            if (j + 1 < queue.messages.size())
            {
                out << ',';
            }
            out << '\n';
        }
        if (!queue.messages.empty())
        {
            write_indent(out, indent + 4);
        }
        out << "]\n";
        write_indent(out, indent + 2);
        out << "}";
        if (i + 1 < queues.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!queues.empty())
    {
        write_indent(out, indent);
    }
    out << "]";
}

void write_leases(
    std::ostream& out,
    const std::vector<statespec::LeaseDecl>& leases,
    int indent
)
{
    out << "[";
    if (!leases.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < leases.size(); ++i)
    {
        const auto& lease = leases[i];
        write_indent(out, indent + 2);
        out << "{\"name\": ";
        write_json_string(out, lease.name);
        out << ", \"resource\": ";
        write_json_optional_string(out, lease.resource);
        out << ", \"ttl\": ";
        write_json_optional_string(out, lease.ttl);
        out << ", \"renew_every\": ";
        write_json_optional_string(out, lease.renew_every);
        out << ", \"holder\": ";
        write_json_optional_string(out, lease.holder);
        out << ", \"fencing_token\": ";
        write_json_optional_bool(out, lease.fencing_token);
        out << ", \"max_ttl\": ";
        write_json_optional_string(out, lease.max_ttl);
        out << "}";
        if (i + 1 < leases.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!leases.empty())
    {
        write_indent(out, indent);
    }
    out << "]";
}

void write_workers(
    std::ostream& out,
    const std::vector<statespec::WorkerDecl>& workers,
    int indent
)
{
    out << "[";
    if (!workers.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < workers.size(); ++i)
    {
        const auto& worker = workers[i];
        write_indent(out, indent + 2);
        out << "{\"name\": ";
        write_json_string(out, worker.name);
        out << ", \"singleton\": ";
        write_json_optional_bool(out, worker.singleton);
        out << ", \"lease\": ";
        write_json_optional_string(out, worker.lease);
        out << ", \"polls\": ";
        write_json_optional_string(out, worker.polls);
        out << ", \"executes\": ";
        write_json_optional_string(out, worker.executes);
        out << ", \"concurrency\": ";
        write_json_optional_int(out, worker.concurrency);
        out << "}";
        if (i + 1 < workers.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!workers.empty())
    {
        write_indent(out, indent);
    }
    out << "]";
}

void write_apis(
    std::ostream& out,
    const std::vector<statespec::ApiDecl>& apis,
    int indent
)
{
    out << "[";
    if (!apis.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < apis.size(); ++i)
    {
        const auto& api = apis[i];
        write_indent(out, indent + 2);
        out << "{\"name\": ";
        write_json_string(out, api.name);
        out << ", \"method\": ";
        write_json_optional_string(out, api.method);
        out << ", \"path\": ";
        write_json_optional_string(out, api.path);
        out << ", \"input\": ";
        write_json_optional_string(out, api.input);
        out << ", \"output\": ";
        write_json_optional_string(out, api.output);
        out << ", \"error\": ";
        write_json_optional_string(out, api.error);
        out << ", \"starts_workflow\": ";
        write_json_optional_string(out, api.starts_workflow);
        out << ", \"enqueues\": ";
        write_json_optional_string(out, api.enqueues);
        out << "}";
        if (i + 1 < apis.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!apis.empty())
    {
        write_indent(out, indent);
    }
    out << "]";
}

void write_workflows(
    std::ostream& out,
    const std::vector<statespec::WorkflowDecl>& workflows,
    int indent
)
{
    out << "[";
    if (!workflows.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < workflows.size(); ++i)
    {
        const auto& workflow = workflows[i];
        write_indent(out, indent + 2);
        out << "{\n";
        write_indent(out, indent + 4);
        out << "\"name\": ";
        write_json_string(out, workflow.name);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"version\": ";
        write_json_optional_int(out, workflow.version);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"singleton\": ";
        write_json_optional_bool(out, workflow.singleton);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"expected_execution_time\": ";
        write_json_optional_string(out, workflow.expected_execution_time);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"start_step\": ";
        write_json_optional_string(out, workflow.start_step);
        out << ",\n";
        write_indent(out, indent + 4);
        out << "\"steps\": [";
        if (!workflow.steps.empty())
        {
            out << '\n';
        }
        for (std::size_t j = 0; j < workflow.steps.size(); ++j)
        {
            const auto& step = workflow.steps[j];
            write_indent(out, indent + 6);
            out << "{\"name\": ";
            write_json_string(out, step.name);
            out << ", \"expected_execution_time\": ";
            write_json_optional_string(out, step.expected_execution_time);
            out << ", \"max_retries\": ";
            write_json_optional_int(out, step.max_retries);
            out << "}";
            if (j + 1 < workflow.steps.size())
            {
                out << ',';
            }
            out << '\n';
        }
        if (!workflow.steps.empty())
        {
            write_indent(out, indent + 4);
        }
        out << "]\n";
        write_indent(out, indent + 2);
        out << "}";
        if (i + 1 < workflows.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!workflows.empty())
    {
        write_indent(out, indent);
    }
    out << "]";
}

void write_policies(
    std::ostream& out,
    const std::vector<statespec::PolicyDecl>& policies,
    int indent
)
{
    out << "[";
    if (!policies.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < policies.size(); ++i)
    {
        const auto& policy = policies[i];
        write_indent(out, indent + 2);
        out << "{\"name\": ";
        write_json_string(out, policy.name);
        out << ", \"tenant_scoped_by\": ";
        write_json_optional_string(out, policy.tenant_scoped_by);
        out << ", \"allows\": [";
        for (std::size_t j = 0; j < policy.allows.size(); ++j)
        {
            if (j > 0)
            {
                out << ", ";
            }
            out << "{\"action\": ";
            write_json_string(out, policy.allows[j].action);
            out << ", \"condition\": ";
            write_json_string(out, policy.allows[j].condition);
            out << "}";
        }
        out << "], \"denies\": [";
        for (std::size_t j = 0; j < policy.denies.size(); ++j)
        {
            if (j > 0)
            {
                out << ", ";
            }
            out << "{\"action\": ";
            write_json_string(out, policy.denies[j].action);
            out << ", \"condition\": ";
            write_json_string(out, policy.denies[j].condition);
            out << "}";
        }
        out << "], \"quotas\": [";
        for (std::size_t j = 0; j < policy.quotas.size(); ++j)
        {
            if (j > 0)
            {
                out << ", ";
            }
            out << "{\"name\": ";
            write_json_string(out, policy.quotas[j].name);
            out << ", \"expression\": ";
            write_json_string(out, policy.quotas[j].expression);
            out << "}";
        }
        out << "], \"audits\": ";
        write_json_string_array(out, policy.audits, indent + 2);
        out << "}";
        if (i + 1 < policies.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!policies.empty())
    {
        write_indent(out, indent);
    }
    out << "]";
}

void write_generators(
    std::ostream& out,
    const std::vector<statespec::GenerateDecl>& generators,
    int indent
)
{
    out << "[";
    if (!generators.empty())
    {
        out << '\n';
    }
    for (std::size_t i = 0; i < generators.size(); ++i)
    {
        const auto& generator = generators[i];
        write_indent(out, indent + 2);
        out << "{\"target\": ";
        write_json_string(out, generator.target);
        out << ", \"out\": ";
        write_json_optional_string(out, generator.out);
        out << ", \"language\": ";
        write_json_optional_string(out, generator.language);
        out << ", \"package\": ";
        write_json_optional_string(out, generator.package);
        out << ", \"runtime\": ";
        write_json_optional_string(out, generator.runtime);
        out << "}";
        if (i + 1 < generators.size())
        {
            out << ',';
        }
        out << '\n';
    }
    if (!generators.empty())
    {
        write_indent(out, indent);
    }
    out << "]";
}

void write_spec_json(
    std::ostream& out,
    const statespec::Spec& spec
)
{
    out << "{\n";
    write_indent(out, 2);
    out << "\"version\": ";
    write_json_optional_string(out, spec.version);
    out << ",\n";

    write_indent(out, 2);
    out << "\"imports\": [";
    for (std::size_t i = 0; i < spec.imports.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << "{\"name\": ";
        write_json_string(out, spec.imports[i].name);
        out << ", \"alias\": ";
        write_json_optional_string(out, spec.imports[i].alias);
        out << "}";
    }
    out << "],\n";

    write_indent(out, 2);
    out << "\"system\": ";
    if (!spec.system.has_value())
    {
        out << "null\n";
        out << "}\n";
        return;
    }

    const auto& system = *spec.system;
    out << "{\n";
    write_indent(out, 4);
    out << "\"name\": ";
    write_json_string(out, system.name);
    out << ",\n";

    write_indent(out, 4);
    out << "\"entities\": ";
    write_entities(out, system.entities, 4);
    out << ",\n";

    write_indent(out, 4);
    out << "\"queues\": ";
    write_queues(out, system.queues, 4);
    out << ",\n";

    write_indent(out, 4);
    out << "\"leases\": ";
    write_leases(out, system.leases, 4);
    out << ",\n";

    write_indent(out, 4);
    out << "\"workers\": ";
    write_workers(out, system.workers, 4);
    out << ",\n";

    write_indent(out, 4);
    out << "\"apis\": ";
    write_apis(out, system.apis, 4);
    out << ",\n";

    write_indent(out, 4);
    out << "\"workflows\": ";
    write_workflows(out, system.workflows, 4);
    out << ",\n";

    write_indent(out, 4);
    out << "\"policies\": ";
    write_policies(out, system.policies, 4);
    out << ",\n";

    write_indent(out, 4);
    out << "\"generators\": ";
    write_generators(out, system.generators, 4);
    out << '\n';

    write_indent(out, 2);
    out << "}\n";
    out << "}\n";
}

int ast_file(const std::string& path)
{
    statespec::DiagnosticBag diagnostics;
    auto tokens = lex_file(path, diagnostics);

    statespec::Parser parser{std::move(tokens)};
    auto spec = parser.parse(diagnostics);

    if (diagnostics.has_errors())
    {
        print_diagnostics(diagnostics);
        return 1;
    }

    write_spec_json(std::cout, spec);
    return 0;
}

int tokens_file(const std::string& path)
{
    statespec::DiagnosticBag diagnostics;
    const auto tokens = lex_file(path, diagnostics);

    for (const auto& token : tokens)
    {
        std::cout << std::setw(4) << token.range.begin.line << ':' << std::setw(3)
                  << token.range.begin.column << "  " << statespec::token_kind_name(token.kind);
        if (!token.lexeme.empty())
        {
            std::cout << "  " << token.lexeme;
        }
        std::cout << '\n';
    }

    if (diagnostics.has_errors())
    {
        print_diagnostics(diagnostics);
        return 1;
    }

    return 0;
}

int validate_file(const std::string& path)
{
    statespec::DiagnosticBag diagnostics;
    auto tokens = lex_file(path, diagnostics);

    statespec::Parser parser{std::move(tokens)};
    auto spec = parser.parse(diagnostics);

    statespec::Validator validator;
    validator.validate(spec, diagnostics);

    if (diagnostics.has_errors())
    {
        std::cout << "invalid\n";
        print_diagnostics(diagnostics);
        return 1;
    }

    std::cout << "valid\n";
    return 0;
}

} // namespace

int main(
    int argc,
    char** argv
)
{
    try
    {
        if (argc != 3)
        {
            std::cerr << "usage: statespec <validate|tokens|ast> <file.sspec>\n";
            return 2;
        }

        const std::string command = argv[1];
        const std::string path = argv[2];

        if (command == "validate")
        {
            return validate_file(path);
        }
        if (command == "tokens")
        {
            return tokens_file(path);
        }
        if (command == "ast")
        {
            return ast_file(path);
        }

        std::cerr << "unknown command: " << command << "\n";
        return 2;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "statespec: " << ex.what() << "\n";
        return 2;
    }
}
