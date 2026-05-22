#include "generator_java_descriptors.hpp"

#include "generator_java_descriptor_areas.hpp"
#include "generator_java_descriptor_support.hpp"
#include "identifier_case.hpp"
#include "type_syntax.hpp"

#include <sstream>
#include <vector>

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

std::string java_decode_func(const IrField& field)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return "decodeBoolean";
    }
    if (base == "int" || base == "int32")
    {
        return "decodeInteger";
    }
    if (base == "int64" || base == "long")
    {
        return "decodeLong";
    }
    if (base == "double" || base == "decimal")
    {
        return "decodeDouble";
    }
    if (base == "json")
    {
        return "decodeJson";
    }
    return "decodeString";
}

std::string java_encode_expr(
    const IrField& field,
    const std::string& accessor
)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return "Json.bool(" + accessor + ")";
    }
    if (base == "int" || base == "int32" || base == "int64" || base == "long")
    {
        return "Json.integer(" + accessor + ")";
    }
    if (base == "double" || base == "decimal")
    {
        return "Json.decimal(java.math.BigDecimal.valueOf(" + accessor + "))";
    }
    if (base == "json")
    {
        return accessor;
    }
    return "Json.string(" + accessor + ")";
}

std::string java_default_response_expr(
    const IrField& field,
    const std::string& context_expr
)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return field.name == "accepted" ? "true" : "false";
    }
    if (base == "int" || base == "int32")
    {
        return "0";
    }
    if (base == "int64" || base == "long")
    {
        return "0L";
    }
    if (base == "double" || base == "decimal")
    {
        return "0.0";
    }
    if (base == "json")
    {
        return context_expr + ".body()";
    }
    if (field.name == "workflow_execution_id" || field.name == "message_id")
    {
        return context_expr + ".apiName() + \":\" + " + context_expr + ".body().canonicalString()";
    }
    if (field.name == "status")
    {
        return "\"Accepted\"";
    }
    return "\"\"";
}

} // namespace

std::string generate_descriptors_java(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    std::ostringstream out;
    out << generate_java_descriptor_prelude(
        system, templates.load("generated/ExternalSystemRuntime.java.tmpl"),
        templates.load("generated/ExternalSystemMetadataRuntime.java.tmpl"),
        templates.load("generated/EntityRepository.java.tmpl")
    );
    out << generate_java_feature_flag_descriptors(system);
    out << generate_java_declaration_descriptors(system);
    out << generate_java_external_system_descriptors(
        system, templates.load("generated/ExternalSystemCallAdapters.java.tmpl")
    );
    out << generate_java_api_descriptors(system);
    out << generate_java_worker_descriptors(system);
    out << generate_java_policy_descriptors(system);
    out << generate_java_shape_descriptors(system);
    out << generate_java_observability_descriptors(system);
    out << generate_java_entity_descriptors(system);
    out << generate_java_runtime_descriptors(system);
    out << generate_java_observability_registration(system);
    out << generate_java_runtime_registration(system);
    out << "}\n";
    return out.str();
}

std::string generate_workflow_step_handler_keys_java(const IrSystem& system)
{
    std::ostringstream out;
    std::vector<std::string> keys;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            keys.push_back(workflow.name + "." + step.name);
        }
    }
    for (std::size_t i = 0; i < keys.size(); ++i)
    {
        out << "            " << java_string(keys[i]) << (i + 1 < keys.size() ? "," : "") << "\n";
    }
    return out.str();
}

std::string generate_workflow_step_handler_methods_java(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "        void handle" << pascal_identifier(workflow.name + "_" + step.name)
                << "(Context context) throws Exception;\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_dispatch_cases_java(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "            if (record.workflowName().equals(" << java_string(workflow.name)
                << ") && record.currentStep().equals(" << java_string(step.name) << ")) {\n";
            out << "                handler.handle"
                << pascal_identifier(workflow.name + "_" + step.name) << "(context);\n";
            out << "                handled = true;\n";
            out << "            }\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_next_cases_java(const IrSystem& system)
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
                out << "            if (record.workflowName().equals(" << java_string(workflow.name)
                    << ") && record.currentStep().equals(" << java_string(step.name) << ")) {\n";
                out << "                nextStep = Optional.of(" << java_string(*statement.target)
                    << ");\n";
                out << "            }\n";
            }
        }
    }
    return out.str();
}

std::string generate_api_operation_handler_methods_java(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "        Descriptors.ApiResponse handle" << pascal_identifier(api.name)
            << "(Descriptors.ApiRequestContext context) throws Exception;\n";
    }
    return out.str();
}

std::string generate_api_operation_dispatch_cases_java(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "        if (route.get().apiName().equals(" << java_string(api.name) << ")) {\n";
        out << "            return Optional.of(handler.handle" << pascal_identifier(api.name)
            << "(context));\n";
        out << "        }\n";
    }
    return out.str();
}

std::string generate_api_operation_default_handler_methods_java(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "        @Override\n";
        out << "        public Descriptors.ApiResponse handle" << pascal_identifier(api.name)
            << "(Descriptors.ApiRequestContext context) {\n";
        if (api.input.has_value())
        {
            out << "            var request = ApiCodecs.decode" << pascal_identifier(api.name)
                << "Request(context);\n";
            out << "            java.util.Objects.requireNonNull(request);\n";
        }
        const auto status_code =
            api.starts_workflow.has_value() || api.enqueues.has_value() ? 202 : 200;
        if (api.output.has_value())
        {
            if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
            {
                out << "            var response = new Descriptors."
                    << pascal_identifier(shape->name) << "(\n";
                for (std::size_t i = 0; i < shape->fields.size(); ++i)
                {
                    const auto& field = shape->fields[i];
                    out << "                ";
                    if (is_optional_type(field.type))
                    {
                        out << "Optional.of(" << java_default_response_expr(field, "context")
                            << ")";
                    }
                    else
                    {
                        out << java_default_response_expr(field, "context");
                    }
                    out << (i + 1 < shape->fields.size() ? "," : "") << "\n";
                }
                out << "            );\n";
                out << "            return ApiCodecs.encode" << pascal_identifier(api.name)
                    << "Response(response, " << status_code << ");\n";
            }
            else
            {
                out << "            return new Descriptors.ApiResponse(" << status_code
                    << ", com.statespec.backend.Json.object(java.util.Map.of()));\n";
            }
        }
        else
        {
            out << "            return new Descriptors.ApiResponse(" << status_code
                << ", com.statespec.backend.Json.object(java.util.Map.of()));\n";
        }
        out << "        }\n\n";
    }
    return out.str();
}

std::string generate_api_codecs_java(const IrSystem& system)
{
    std::ostringstream out;
    out << "    private static Json requireMember(Json value, String fieldName) {\n";
    out << "        return value.find(fieldName).filter(member -> !(member instanceof "
           "Json.NullValue)).orElseThrow(() -> new IllegalArgumentException(\"missing required "
           "API field \" + fieldName));\n";
    out << "    }\n\n";
    out << "    private static String decodeString(Json value, String fieldName) {\n";
    out << "        if (value instanceof Json.StringValue stringValue) { return "
           "stringValue.value(); }\n";
    out << "        throw new IllegalArgumentException(\"API field \" + fieldName + \" must be a "
           "string\");\n";
    out << "    }\n\n";
    out << "    private static Boolean decodeBoolean(Json value, String fieldName) {\n";
    out << "        if (value instanceof Json.BooleanValue booleanValue) { return "
           "booleanValue.value(); }\n";
    out << "        throw new IllegalArgumentException(\"API field \" + fieldName + \" must be a "
           "bool\");\n";
    out << "    }\n\n";
    out << "    private static Long decodeLong(Json value, String fieldName) {\n";
    out << "        if (value instanceof Json.IntegerValue integerValue) { return "
           "integerValue.value(); }\n";
    out << "        throw new IllegalArgumentException(\"API field \" + fieldName + \" must be an "
           "integer\");\n";
    out << "    }\n\n";
    out << "    private static Integer decodeInteger(Json value, String fieldName) {\n";
    out << "        return Math.toIntExact(decodeLong(value, fieldName));\n";
    out << "    }\n\n";
    out << "    private static Double decodeDouble(Json value, String fieldName) {\n";
    out << "        if (value instanceof Json.DecimalValue decimalValue) { return "
           "decimalValue.value().doubleValue(); }\n";
    out << "        if (value instanceof Json.IntegerValue integerValue) { return "
           "(double) integerValue.value(); }\n";
    out << "        throw new IllegalArgumentException(\"API field \" + fieldName + \" must be a "
           "number\");\n";
    out << "    }\n\n";
    out << "    private static Json decodeJson(Json value, String fieldName) {\n";
    out << "        return value;\n";
    out << "    }\n\n";

    for (const auto& api : system.apis)
    {
        if (api.input.has_value())
        {
            const auto* shape = find_shape(system, *api.input);
            if (shape != nullptr)
            {
                out << "    public static Descriptors." << pascal_identifier(shape->name)
                    << " decode" << pascal_identifier(api.name)
                    << "Request(Descriptors.ApiRequestContext request) {\n";
                out << "        return new Descriptors." << pascal_identifier(shape->name) << "(\n";
                for (std::size_t i = 0; i < shape->fields.size(); ++i)
                {
                    const auto& field = shape->fields[i];
                    out << "            ";
                    if (is_optional_type(field.type))
                    {
                        out << "request.body().find(" << java_string(field.name)
                            << ").filter(member -> !(member instanceof Json.NullValue)).map(member "
                               "-> Optional.of("
                            << java_decode_func(field) << "(member, " << java_string(field.name)
                            << "))).orElse(Optional.empty())";
                    }
                    else
                    {
                        out << java_decode_func(field) << "(requireMember(request.body(), "
                            << java_string(field.name) << "), " << java_string(field.name) << ")";
                    }
                    out << (i + 1 < shape->fields.size() ? "," : "") << "\n";
                }
                out << "        );\n";
                out << "    }\n\n";
            }
        }
        if (api.output.has_value())
        {
            const auto* shape = find_shape(system, *api.output);
            if (shape != nullptr)
            {
                out << "    public static Descriptors." << pascal_identifier(shape->name)
                    << " decode" << pascal_identifier(api.name)
                    << "Response(Descriptors.ApiResponse response) {\n";
                out << "        return new Descriptors." << pascal_identifier(shape->name) << "(\n";
                for (std::size_t i = 0; i < shape->fields.size(); ++i)
                {
                    const auto& field = shape->fields[i];
                    out << "            ";
                    if (is_optional_type(field.type))
                    {
                        out << "response.body().find(" << java_string(field.name)
                            << ").filter(member -> !(member instanceof Json.NullValue)).map(member "
                               "-> Optional.of("
                            << java_decode_func(field) << "(member, " << java_string(field.name)
                            << "))).orElse(Optional.empty())";
                    }
                    else
                    {
                        out << java_decode_func(field) << "(requireMember(response.body(), "
                            << java_string(field.name) << "), " << java_string(field.name) << ")";
                    }
                    out << (i + 1 < shape->fields.size() ? "," : "") << "\n";
                }
                out << "        );\n";
                out << "    }\n\n";

                out << "    public static Descriptors.ApiResponse encode"
                    << pascal_identifier(api.name) << "Response(Descriptors."
                    << pascal_identifier(shape->name) << " response) {\n";
                out << "        return encode" << pascal_identifier(api.name)
                    << "Response(response, 200);\n";
                out << "    }\n\n";
                out << "    public static Descriptors.ApiResponse encode"
                    << pascal_identifier(api.name) << "Response(Descriptors."
                    << pascal_identifier(shape->name) << " response, int statusCode) {\n";
                out << "        Map<String, Json> body = new TreeMap<>();\n";
                for (const auto& field : shape->fields)
                {
                    const auto accessor = "response." + field.name + "()";
                    if (is_optional_type(field.type))
                    {
                        out << "        " << accessor << ".ifPresent(value -> body.put("
                            << java_string(field.name) << ", " << java_encode_expr(field, "value")
                            << "));\n";
                    }
                    else
                    {
                        out << "        body.put(" << java_string(field.name) << ", "
                            << java_encode_expr(field, accessor) << ");\n";
                    }
                }
                out << "        return new Descriptors.ApiResponse(statusCode, "
                       "Json.object(body));\n";
                out << "    }\n\n";
            }
        }
    }
    return out.str();
}

} // namespace statespec
