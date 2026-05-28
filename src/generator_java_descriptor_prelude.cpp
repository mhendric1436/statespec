#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <algorithm>
#include <sstream>

namespace statespec
{

namespace
{

bool java_shape_is_api_contract(
    const IrSystem& system,
    std::string_view shape_name
)
{
    return std::any_of(
        system.apis.begin(), system.apis.end(),
        [&](const auto& api)
        {
            return (api.input.has_value() && *api.input == shape_name) ||
                   (api.output.has_value() && *api.output == shape_name);
        }
    );
}

bool java_has_common_shapes(const IrSystem& system)
{
    return std::any_of(
        system.shapes.begin(), system.shapes.end(),
        [&](const auto& shape) { return !java_shape_is_api_contract(system, shape.name); }
    );
}

std::string java_descriptor_module_imports(const IrSystem& system)
{
    std::ostringstream out;
    out << "import com.statespec.generated.descriptors.CoreDescriptorModule;\n";
    out << "import com.statespec.generated.descriptors.EventDescriptorModule;\n";
    out << "import com.statespec.generated.descriptors.ExternalSystemDescriptorModule;\n";
    out << "import com.statespec.generated.descriptors.RuntimeDescriptorModule;\n";
    if (java_has_common_shapes(system))
    {
        out << "import com.statespec.generated.descriptors.ShapeDescriptorModule;\n";
    }
    for (const auto& workflow : system.workflows)
    {
        out << "import com.statespec.generated.workflows." << pascal_identifier(workflow.name)
            << "DescriptorModule;\n";
    }
    return out.str();
}

} // namespace

std::string generate_java_descriptor_prelude(
    const IrSystem& system,
    const std::string&,
    const std::string&,
    const std::string& entity_repository_runtime
)
{
    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << java_descriptor_module_imports(system);
    out << "import com.statespec.generated.descriptors.types.*;\n";
    out << "import com.statespec.backend.Json;\n";
    out << "import com.statespec.backend.Queue;\n";
    out << "import com.statespec.backend.Workflow;\n";
    out << "import com.statespec.backend.Backend.FieldDescriptor;\n";
    out << "import com.statespec.backend.Backend.FieldType;\n";
    out << "import com.statespec.backend.Queue.QueueDefinition;\n";
    out << "import com.statespec.backend.Workflow.WorkflowDefinition;\n";
    out << "import java.time.Duration;\n";
    out << "import java.util.List;\n";
    out << "import java.util.Map;\n";
    out << "import java.util.Optional;\n";
    out << "\n";
    out << "public final class Descriptors {\n";
    out << "    private Descriptors() {}\n\n";
    for (const auto& event : system.events)
    {
        out << "    public static EventEnvelope build" << pascal_identifier(event.name)
            << "Event(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "        Json " << lower_camel_identifier(field.name);
            out << (i + 1 < event.fields.size() ? "," : "") << "\n";
        }
        out << "    ) {\n";
        out << "        return EventDescriptorModule.build" << pascal_identifier(event.name)
            << "Event(";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << lower_camel_identifier(event.fields[i].name);
        }
        out << ");\n";
        out << "    }\n\n";
    }

    out << entity_repository_runtime << "\n";

    return out.str();
}

} // namespace statespec
