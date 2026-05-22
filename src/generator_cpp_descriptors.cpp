#include "generator_cpp_descriptors.hpp"

#include "generator_cpp_descriptor_areas.hpp"
#include "generator_cpp_descriptor_support.hpp"
#include "identifier_case.hpp"
#include "type_syntax.hpp"

#include <sstream>

namespace statespec
{
namespace
{

const IrShape* find_shape(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& shape : system.shapes)
    {
        if (shape.name == name)
        {
            return &shape;
        }
    }
    return nullptr;
}

const IrEntity* find_entity(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& entity : system.entities)
    {
        if (entity.name == name)
        {
            return &entity;
        }
    }
    return nullptr;
}

const IrField* find_field(
    const IrShape& shape,
    const std::string& name
)
{
    for (const auto& field : shape.fields)
    {
        if (field.name == name)
        {
            return &field;
        }
    }
    return nullptr;
}

const IrEntity* create_entity_for_api(
    const IrSystem& system,
    const IrApi& api
)
{
    constexpr std::string_view prefix = "Create";
    if (api.method.value_or("") != "POST" || api.name.rfind(prefix, 0) != 0 || !api.input)
    {
        return nullptr;
    }
    return find_entity(system, api.name.substr(prefix.size()));
}

bool create_api_has_required_request_fields(
    const IrEntity& entity,
    const IrShape& request
)
{
    for (const auto& field : entity.fields)
    {
        if (field.name == "created_at" || field.name == "updated_at" || field.name == "status")
        {
            continue;
        }
        if (find_field(request, field.name) == nullptr)
        {
            return false;
        }
    }
    return true;
}

std::string cpp_decode_expr(
    const IrField& field,
    std::string value_expr
)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return "decode_bool(" + value_expr + ", " + cpp_string(field.name) + ")";
    }
    if (base == "int" || base == "int32")
    {
        return "static_cast<std::int32_t>(decode_int64(" + value_expr + ", " +
               cpp_string(field.name) + "))";
    }
    if (base == "int64" || base == "long")
    {
        return "decode_int64(" + value_expr + ", " + cpp_string(field.name) + ")";
    }
    if (base == "double" || base == "decimal")
    {
        return "decode_double(" + value_expr + ", " + cpp_string(field.name) + ")";
    }
    if (base == "json")
    {
        return value_expr;
    }
    return "decode_string(" + value_expr + ", " + cpp_string(field.name) + ")";
}

std::string cpp_json_expr(
    const IrField& field,
    const std::string& accessor
)
{
    const auto base = strip_optional_type(field.type);
    if (base == "json")
    {
        return accessor;
    }
    return "statespec::backend::Json{" + accessor + "}";
}

std::string cpp_default_response_assignment(
    const IrField& field,
    const std::string& context_expr
)
{
    const auto base = strip_optional_type(field.type);
    const std::string prefix = is_optional_type(field.type) ? "std::optional{" : "";
    const std::string suffix = is_optional_type(field.type) ? "}" : "";
    if (base == "bool")
    {
        return prefix + std::string(field.name == "accepted" ? "true" : "false") + suffix;
    }
    if (base == "int" || base == "int32")
    {
        return prefix + "std::int32_t{0}" + suffix;
    }
    if (base == "int64" || base == "long")
    {
        return prefix + "std::int64_t{0}" + suffix;
    }
    if (base == "double" || base == "decimal")
    {
        return prefix + "0.0" + suffix;
    }
    if (base == "json")
    {
        return prefix + context_expr + ".body" + suffix;
    }
    if (field.name == "workflow_execution_id")
    {
        return prefix + context_expr + ".api_name + \":\" + " + context_expr +
               ".body.canonical_string()" + suffix;
    }
    if (field.name == "message_id")
    {
        return prefix + context_expr + ".api_name + \":\" + " + context_expr +
               ".body.canonical_string()" + suffix;
    }
    if (field.name == "status")
    {
        return prefix + "std::string{\"Accepted\"}" + suffix;
    }
    return prefix + "std::string{}" + suffix;
}

std::string cpp_create_response_assignment(
    const IrField& field,
    const IrShape& request,
    const IrEntity& entity,
    const std::string& context_expr
)
{
    if (find_field(request, field.name) != nullptr)
    {
        return "request." + field.name;
    }
    if (field.name == "status")
    {
        return "std::string{" + cpp_string(entity.initial_state.value_or("Created")) + "}";
    }
    return cpp_default_response_assignment(field, context_expr);
}

void write_cpp_parent_validation(
    std::ostringstream& out,
    const IrSystem& system,
    const IrEntity& entity,
    const IrShape& request
)
{
    for (const auto& relation : entity.relations)
    {
        if (relation.kind != "parent" || relation.optional)
        {
            continue;
        }
        const auto* parent = find_entity(system, relation.target);
        if (parent == nullptr)
        {
            continue;
        }
        out << "        {\n";
        out << "            Default" << pascal_identifier(parent->name) << "Repository parent;\n";
        out << "            const auto parent_record = parent.getTx(\n";
        out << "                *tx,\n";
        out << "                {\n";
        for (const auto& key_field : parent->key_fields)
        {
            const auto* request_field = find_field(request, key_field);
            if (request_field == nullptr)
            {
                out << "                    EntityKeyValue{" << cpp_string(key_field)
                    << ", statespec::backend::Json::null()},\n";
            }
            else
            {
                out << "                    EntityKeyValue{" << cpp_string(key_field) << ", "
                    << cpp_json_expr(*request_field, "request." + key_field) << "},\n";
            }
        }
        out << "                }\n";
        out << "            );\n";
        out << "            if (!parent_record.has_value())\n";
        out << "            {\n";
        out << "                throw statespec::backend::BackendError(\"missing required parent "
            << parent->name << "\");\n";
        out << "            }\n";
        out << "        }\n";
    }
}

bool write_cpp_create_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = create_entity_for_api(system, api);
    const auto* request = api.input ? find_shape(system, *api.input) : nullptr;
    if (entity == nullptr || request == nullptr)
    {
        return false;
    }
    if (!create_api_has_required_request_fields(*entity, *request))
    {
        out << "        throw statespec::backend::BackendError("
            << cpp_string(
                   "generated create handler for " + api.name +
                   " requires request fields for every non-foundational " + entity->name +
                   " entity field"
               )
            << ");\n";
        return true;
    }

    const auto status = entity->initial_state.value_or("Created");
    out << "        const auto request = decode_" << snake_identifier(api.name)
        << "_request(context);\n";
    out << "        Default" << pascal_identifier(entity->name) << "Repository repository;\n";
    out << "        repository.register_descriptor(backend_);\n";
    out << "        auto tx = backend_.begin();\n";
    out << "        try\n";
    out << "        {\n";
    write_cpp_parent_validation(out, system, *entity, *request);
    out << "            statespec::backend::Json::Object document;\n";
    for (const auto& field : entity->fields)
    {
        if (field.name == "created_at" || field.name == "updated_at")
        {
            out << "            document[" << cpp_string(field.name)
                << "] = statespec::backend::Json{generated_api_timestamp()};\n";
        }
        else if (field.name == "status")
        {
            out << "            document[" << cpp_string(field.name)
                << "] = statespec::backend::Json{" << cpp_string(status) << "};\n";
        }
        else if (const auto* request_field = find_field(*request, field.name);
                 request_field != nullptr)
        {
            out << "            document[" << cpp_string(field.name)
                << "] = " << cpp_json_expr(*request_field, "request." + field.name) << ";\n";
        }
    }
    out << "            const auto record = repository.createTx(\n";
    out << "                *tx, statespec::backend::Json::object(std::move(document))\n";
    out << "            );\n";
    out << "            if (!record.has_value())\n";
    out << "            {\n";
    out << "                throw statespec::backend::BackendError(\"entity create failed\");\n";
    out << "            }\n";
    out << "            backend_.commit(*tx);\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "            ::statespec_generated::" << pascal_identifier(shape->name)
                << " response{};\n";
            for (const auto& field : shape->fields)
            {
                out << "            response." << field.name << " = "
                    << cpp_create_response_assignment(field, *request, *entity, "context") << ";\n";
            }
            out << "            return encode_" << snake_identifier(api.name)
                << "_response(response, 201);\n";
        }
        else
        {
            out << "            return ApiResponse{201, statespec::backend::Json::object({})};\n";
        }
    }
    else
    {
        out << "            return ApiResponse{201, statespec::backend::Json::object({})};\n";
    }
    out << "        }\n";
    out << "        catch (...)\n";
    out << "        {\n";
    out << "            tx->abort();\n";
    out << "            throw;\n";
    out << "        }\n";
    return true;
}

} // namespace

std::string generate_system_descriptors_header(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    std::ostringstream out;
    out << generate_cpp_descriptor_prelude(
        system, templates.load("generated/external_system_runtime.hpp.tmpl"),
        templates.load("generated/external_system_metadata_runtime.hpp.tmpl"),
        templates.load("generated/entity_repository.hpp.tmpl")
    );
    out << generate_cpp_feature_flag_descriptors(system);
    out << generate_cpp_declaration_descriptors(system);
    out << generate_cpp_external_system_descriptors(
        system, templates.load("generated/external_system_call_adapters.hpp.tmpl")
    );
    out << generate_cpp_api_descriptors(system);
    out << generate_cpp_worker_descriptors(system);
    out << generate_cpp_policy_descriptors(system);
    out << generate_cpp_shape_descriptors(system);
    out << generate_cpp_observability_descriptors(system);
    out << generate_cpp_entity_descriptors(system);
    out << generate_cpp_runtime_descriptors(system);
    out << generate_cpp_observability_registration(system);
    out << generate_cpp_runtime_registration(system, templates);
    out << "} // namespace statespec_generated\n";
    return out.str();
}

std::string generate_workflow_step_handler_keys(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "        " << cpp_string(workflow.name + "." + step.name) << ",\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_handler_methods(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "    virtual void handle_" << snake_identifier(workflow.name + "_" + step.name)
                << "(const WorkflowStepHandlerContext& context) = 0;\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_dispatch_cases(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "            if (record.workflow_name == " << cpp_string(workflow.name)
                << " && record.current_step == " << cpp_string(step.name) << ")\n";
            out << "            {\n";
            out << "                handler_.handle_"
                << snake_identifier(workflow.name + "_" + step.name) << "(context);\n";
            out << "                handled = true;\n";
            out << "            }\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_next_cases(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            for (const auto& statement : step.statements)
            {
                if (statement.kind != "transition_to" || !statement.target.has_value())
                {
                    continue;
                }
                out << "            if (record.workflow_name == " << cpp_string(workflow.name)
                    << " && record.current_step == " << cpp_string(step.name) << ")\n";
                out << "            {\n";
                out << "                next_step = " << cpp_string(*statement.target) << ";\n";
                out << "            }\n";
            }
        }
    }
    return out.str();
}

std::string generate_api_operation_handler_methods(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "    virtual ApiResponse handle_" << snake_identifier(api.name)
            << "(const ApiRequestContext& context) = 0;\n";
    }
    return out.str();
}

std::string generate_api_operation_dispatch_cases(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "    if (route->api_name == " << cpp_string(api.name) << ")\n";
        out << "    {\n";
        out << "        return handler.handle_" << snake_identifier(api.name) << "(context);\n";
        out << "    }\n";
    }
    return out.str();
}

std::string generate_api_operation_default_handler_methods(const IrSystem& system)
{
    std::ostringstream out;
    out << "    static std::string generated_api_timestamp()\n";
    out << "    {\n";
    out << "        return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(\n";
    out << "            std::chrono::system_clock::now().time_since_epoch()).count());\n";
    out << "    }\n\n";
    for (const auto& api : system.apis)
    {
        out << "    ApiResponse handle_" << snake_identifier(api.name)
            << "(const ApiRequestContext& context) override\n";
        out << "    {\n";
        if (write_cpp_create_handler_body(out, system, api))
        {
            out << "    }\n\n";
            continue;
        }
        if (api.input.has_value())
        {
            out << "        const auto request = decode_" << snake_identifier(api.name)
                << "_request(context);\n";
            out << "        (void)request;\n";
        }
        if (api.output.has_value())
        {
            if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
            {
                out << "        ::statespec_generated::" << pascal_identifier(shape->name)
                    << " response{};\n";
                for (const auto& field : shape->fields)
                {
                    out << "        response." << field.name << " = "
                        << cpp_default_response_assignment(field, "context") << ";\n";
                }
                out << "        return encode_" << snake_identifier(api.name)
                    << "_response(response, "
                    << (api.starts_workflow.has_value() || api.enqueues.has_value() ? 202 : 200)
                    << ");\n";
            }
            else
            {
                out << "        return ApiResponse{"
                    << (api.starts_workflow.has_value() || api.enqueues.has_value() ? 202 : 200)
                    << ", statespec::backend::Json::object({})};\n";
            }
        }
        else
        {
            out << "        return ApiResponse{"
                << (api.starts_workflow.has_value() || api.enqueues.has_value() ? 202 : 200)
                << ", statespec::backend::Json::object({})};\n";
        }
        out << "    }\n\n";
    }
    return out.str();
}

std::string generate_api_codecs(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline const statespec::backend::Json& require_member(\n";
    out << "    const statespec::backend::Json& value,\n";
    out << "    std::string_view field_name\n";
    out << ")\n";
    out << "{\n";
    out << "    const auto* member = value.find(field_name);\n";
    out << "    if (member == nullptr || member->is_null())\n";
    out << "    {\n";
    out << "        throw std::runtime_error(\"missing required API field\");\n";
    out << "    }\n";
    out << "    return *member;\n";
    out << "}\n\n";
    out << "inline std::string decode_string(const statespec::backend::Json& value, "
           "std::string_view)\n";
    out << "{\n";
    out << "    return value.as_string();\n";
    out << "}\n\n";
    out << "inline bool decode_bool(const statespec::backend::Json& value, std::string_view)\n";
    out << "{\n";
    out << "    return value.as_bool();\n";
    out << "}\n\n";
    out << "inline std::int64_t decode_int64(const statespec::backend::Json& value, "
           "std::string_view)\n";
    out << "{\n";
    out << "    return value.as_int64();\n";
    out << "}\n\n";
    out << "inline double decode_double(const statespec::backend::Json& value, "
           "std::string_view)\n";
    out << "{\n";
    out << "    return value.as_double();\n";
    out << "}\n\n";
    for (const auto& api : system.apis)
    {
        if (api.input.has_value())
        {
            const auto* shape = find_shape(system, *api.input);
            if (shape != nullptr)
            {
                out << "inline ::statespec_generated::" << pascal_identifier(shape->name)
                    << " decode_" << snake_identifier(api.name)
                    << "_request(const ApiRequestContext& context)\n";
                out << "{\n";
                out << "    ::statespec_generated::" << pascal_identifier(shape->name)
                    << " decoded{};\n";
                for (const auto& field : shape->fields)
                {
                    if (is_optional_type(field.type))
                    {
                        out << "    if (const auto* member = context.body.find("
                            << cpp_string(field.name)
                            << "); member != nullptr && !member->is_null())\n";
                        out << "    {\n";
                        out << "        decoded." << field.name << " = "
                            << cpp_decode_expr(field, "*member") << ";\n";
                        out << "    }\n";
                    }
                    else
                    {
                        out << "    decoded." << field.name << " = "
                            << cpp_decode_expr(
                                   field,
                                   "require_member(context.body, " + cpp_string(field.name) + ")"
                               )
                            << ";\n";
                    }
                }
                out << "    return decoded;\n";
                out << "}\n\n";
            }
        }
        if (api.output.has_value())
        {
            const auto* shape = find_shape(system, *api.output);
            if (shape != nullptr)
            {
                out << "inline ApiResponse encode_" << snake_identifier(api.name)
                    << "_response(const ::statespec_generated::" << pascal_identifier(shape->name)
                    << "& response, int status_code = 200)\n";
                out << "{\n";
                out << "    statespec::backend::Json::Object body;\n";
                for (const auto& field : shape->fields)
                {
                    if (is_optional_type(field.type))
                    {
                        out << "    if (response." << field.name << ".has_value())\n";
                        out << "    {\n";
                        out << "        body.emplace(" << cpp_string(field.name) << ", "
                            << cpp_json_expr(field, "*response." + field.name) << ");\n";
                        out << "    }\n";
                    }
                    else
                    {
                        out << "    body.emplace(" << cpp_string(field.name) << ", "
                            << cpp_json_expr(field, "response." + field.name) << ");\n";
                    }
                }
                out << "    return ApiResponse{status_code, "
                       "statespec::backend::Json::object(std::move(body))};\n";
                out << "}\n\n";

                out << "inline ::statespec_generated::" << pascal_identifier(shape->name)
                    << " decode_" << snake_identifier(api.name)
                    << "_response(const ApiResponse& response)\n";
                out << "{\n";
                out << "    ::statespec_generated::" << pascal_identifier(shape->name)
                    << " decoded{};\n";
                for (const auto& field : shape->fields)
                {
                    if (is_optional_type(field.type))
                    {
                        out << "    if (const auto* member = response.body.find("
                            << cpp_string(field.name)
                            << "); member != nullptr && !member->is_null())\n";
                        out << "    {\n";
                        out << "        decoded." << field.name << " = "
                            << cpp_decode_expr(field, "*member") << ";\n";
                        out << "    }\n";
                    }
                    else
                    {
                        out << "    decoded." << field.name << " = "
                            << cpp_decode_expr(
                                   field,
                                   "require_member(response.body, " + cpp_string(field.name) + ")"
                               )
                            << ";\n";
                    }
                }
                out << "    return decoded;\n";
                out << "}\n\n";
            }
        }
    }
    return out.str();
}

} // namespace statespec
