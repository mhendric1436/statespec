#include "generator_backend.hpp"

namespace statespec::generator_backend
{

namespace
{

std::string generate_qu_manifest(const SystemDecl& system, const GenerateDecl& declaration)
{
    std::ostringstream out;
    append_header(out, system, declaration.target);
    out << "queues:\n";
    for (const auto& queue : system.queues)
    {
        out << "  - name: " << queue.name << "\n";
        out << "    namespace: " << optional_or_empty(queue.namespace_name) << "\n";
        out << "    channel: " << optional_or_empty(queue.channel) << "\n";
        out << "    visibility_timeout: " << optional_or_empty(queue.visibility_timeout) << "\n";
        out << "    max_attempts: " << queue.max_attempts.value_or(0) << "\n";
        out << "    messages:\n";
        for (const auto& message : queue.messages)
        {
            out << "      - name: " << message.name << "\n";
            out << "        payload_struct: " << payload_struct_name(queue, message) << "\n";
            out << "        idempotency_key: " << optional_or_empty(message.idempotency_key) << "\n";
            out << "        payload_fields:\n";
            for (const auto& field : message.payload_fields)
            {
                out << "          - name: " << field.name << "\n";
                out << "            type: " << field.type << "\n";
                out << "            cpp_type: " << cpp_type_for_statespec_type(field.type) << "\n";
                out << "            optional: " << bool_text(is_optional_type(field.type)) << "\n";
            }
        }
    }
    return out.str();
}

std::string generate_qu_messages_header(const SystemDecl& system)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include <chrono>\n";
    out << "#include <cstddef>\n";
    out << "#include <cstdint>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n";
    out << "#include <string_view>\n\n";
    out << "namespace statespec_generated::qu\n";
    out << "{\n\n";
    out << "struct PayloadFieldMetadata\n";
    out << "{\n";
    out << "    std::string_view name;\n";
    out << "    std::string_view statespec_type;\n";
    out << "    std::string_view cpp_type;\n";
    out << "    bool optional;\n";
    out << "};\n\n";
    out << "struct MessageMetadata\n";
    out << "{\n";
    out << "    std::string_view queue_name;\n";
    out << "    std::string_view message_name;\n";
    out << "    std::string_view payload_struct;\n";
    out << "    std::string_view idempotency_key;\n";
    out << "    const PayloadFieldMetadata* payload_fields;\n";
    out << "    std::size_t payload_field_count;\n";
    out << "};\n\n";
    out << "struct QueueMetadata\n";
    out << "{\n";
    out << "    std::string_view name;\n";
    out << "    std::string_view namespace_name;\n";
    out << "    std::string_view channel;\n";
    out << "    std::string_view visibility_timeout;\n";
    out << "    int max_attempts;\n";
    out << "    const MessageMetadata* messages;\n";
    out << "    std::size_t message_count;\n";
    out << "};\n\n";

    for (const auto& queue : system.queues)
    {
        for (const auto& message : queue.messages)
        {
            out << "struct " << payload_struct_name(queue, message) << "\n";
            out << "{\n";
            for (const auto& field : message.payload_fields)
            {
                out << "    " << cpp_type_for_statespec_type(field.type) << ' ' << field.name
                    << "{};\n";
            }
            out << "};\n\n";
        }
    }

    out << "const QueueMetadata* queues();\n";
    out << "std::size_t queue_count();\n";
    out << "const MessageMetadata* messages();\n";
    out << "std::size_t message_count();\n";
    out << "std::string_view idempotency_key_field(";
    out << "std::string_view queue_name, std::string_view message_name);\n\n";
    out << "} // namespace statespec_generated::qu\n";
    return out.str();
}

std::string generate_qu_metadata_source(const SystemDecl& system)
{
    std::ostringstream out;
    out << "#include \"qu_messages.hpp\"\n\n";
    out << "#include <array>\n\n";
    out << "namespace statespec_generated::qu\n";
    out << "{\n";
    out << "namespace\n";
    out << "{\n\n";

    std::size_t total_message_count = 0;
    for (const auto& queue : system.queues)
    {
        for (const auto& message : queue.messages)
        {
            ++total_message_count;
            const auto symbol = to_lower(queue.name + message.name);
            out << "constexpr std::array<PayloadFieldMetadata, " << message.payload_fields.size()
                << "> " << symbol << "_payload_fields{{\n";
            for (const auto& field : message.payload_fields)
            {
                out << "    PayloadFieldMetadata{\"" << field.name << "\", \"" << field.type
                    << "\", \"" << cpp_type_for_statespec_type(field.type) << "\", "
                    << bool_text(is_optional_type(field.type)) << "},\n";
            }
            out << "}};\n\n";
        }
    }

    out << "constexpr std::array<MessageMetadata, " << total_message_count << "> all_messages{{\n";
    for (const auto& queue : system.queues)
    {
        for (const auto& message : queue.messages)
        {
            const auto symbol = to_lower(queue.name + message.name);
            out << "    MessageMetadata{\"" << queue.name << "\", \"" << message.name
                << "\", \"" << payload_struct_name(queue, message) << "\", \""
                << optional_or_empty(message.idempotency_key) << "\", " << symbol
                << "_payload_fields.data(), " << symbol << "_payload_fields.size()},\n";
        }
    }
    out << "}};\n\n";

    for (const auto& queue : system.queues)
    {
        const auto symbol = to_lower(queue.name);
        out << "constexpr std::array<MessageMetadata, " << queue.messages.size() << "> " << symbol
            << "_messages{{\n";
        for (const auto& message : queue.messages)
        {
            const auto message_symbol = to_lower(queue.name + message.name);
            out << "    MessageMetadata{\"" << queue.name << "\", \"" << message.name
                << "\", \"" << payload_struct_name(queue, message) << "\", \""
                << optional_or_empty(message.idempotency_key) << "\", " << message_symbol
                << "_payload_fields.data(), " << message_symbol << "_payload_fields.size()},\n";
        }
        out << "}};\n\n";
    }

    out << "constexpr std::array<QueueMetadata, " << system.queues.size() << "> all_queues{{\n";
    for (const auto& queue : system.queues)
    {
        const auto symbol = to_lower(queue.name);
        out << "    QueueMetadata{\"" << queue.name << "\", \""
            << optional_or_empty(queue.namespace_name) << "\", \"" << optional_or_empty(queue.channel)
            << "\", \"" << optional_or_empty(queue.visibility_timeout) << "\", "
            << queue.max_attempts.value_or(0) << ", " << symbol << "_messages.data(), " << symbol
            << "_messages.size()},\n";
    }
    out << "}};\n\n";

    out << "} // namespace\n\n";
    out << "const QueueMetadata* queues()\n";
    out << "{\n";
    out << "    return all_queues.data();\n";
    out << "}\n\n";
    out << "std::size_t queue_count()\n";
    out << "{\n";
    out << "    return all_queues.size();\n";
    out << "}\n\n";
    out << "const MessageMetadata* messages()\n";
    out << "{\n";
    out << "    return all_messages.data();\n";
    out << "}\n\n";
    out << "std::size_t message_count()\n";
    out << "{\n";
    out << "    return all_messages.size();\n";
    out << "}\n\n";
    out << "std::string_view idempotency_key_field(";
    out << "std::string_view queue_name, std::string_view message_name)\n";
    out << "{\n";
    out << "    for (std::size_t i = 0; i < all_messages.size(); ++i)\n";
    out << "    {\n";
    out << "        const auto& message = all_messages[i];\n";
    out << "        if (message.queue_name == queue_name && message.message_name == message_name)\n";
    out << "        {\n";
    out << "            return message.idempotency_key;\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return {};\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::qu\n";
    return out.str();
}

} // namespace

void generate_qu(
    const SystemDecl& system,
    const GenerateDecl& declaration,
    const GenerationOptions& options,
    GenerationResult& result
)
{
    const auto root = output_root(declaration, options);
    result.files.push_back(
        GeneratedFile{join_path(root, "qu-manifest.yaml"), generate_qu_manifest(system, declaration)}
    );
    result.files.push_back(
        GeneratedFile{join_path(root, "qu_messages.hpp"), generate_qu_messages_header(system)}
    );
    result.files.push_back(
        GeneratedFile{join_path(root, "qu_metadata.cpp"), generate_qu_metadata_source(system)}
    );
}

} // namespace statespec::generator_backend
