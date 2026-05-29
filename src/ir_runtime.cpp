#include "ir_runtime.hpp"

#include "identifier_case.hpp"
#include "ir_lowering_helpers.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace statespec
{

namespace
{

bool has_api(
    const IrSystem& ir,
    const std::string& name
)
{
    return std::any_of(
        ir.apis.begin(), ir.apis.end(), [&](const IrApi& api) { return api.name == name; }
    );
}

bool has_served_api(
    const IrApiServer& api_server,
    const std::string& api_name
)
{
    return std::find(api_server.serves.begin(), api_server.serves.end(), api_name) !=
           api_server.serves.end();
}

const IrShape* find_shape(
    const IrSystem& ir,
    const std::string& name
)
{
    for (const auto& shape : ir.shapes)
    {
        if (shape.name == name)
        {
            return &shape;
        }
    }
    return nullptr;
}

const IrEntity* find_entity(
    const IrSystem& ir,
    const std::string& name
)
{
    for (const auto& entity : ir.entities)
    {
        if (entity.name == name)
        {
            return &entity;
        }
    }
    return nullptr;
}

bool ends_with_suffix(
    const std::string& value,
    std::string_view suffix
)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string list_item_type(const std::string& type)
{
    return ends_with_suffix(type, "[]") ? type.substr(0, type.size() - 2) : std::string{};
}

const IrField* list_response_array_field(const IrShape& shape)
{
    for (const auto& field : shape.fields)
    {
        if (!list_item_type(field.type).empty())
        {
            return &field;
        }
    }
    return nullptr;
}

const IrEntity* response_entity(
    const IrSystem& ir,
    const std::optional<std::string>& output
)
{
    if (!output.has_value() || !ends_with_suffix(*output, "Response"))
    {
        return nullptr;
    }
    return find_entity(ir, output->substr(0, output->size() - std::string_view{"Response"}.size()));
}

const IrEntity* list_response_entity(
    const IrSystem& ir,
    const IrApi& api
)
{
    if (!api.output.has_value())
    {
        return nullptr;
    }
    const auto* shape = find_shape(ir, *api.output);
    const auto* array_field = shape == nullptr ? nullptr : list_response_array_field(*shape);
    if (array_field == nullptr)
    {
        return nullptr;
    }
    auto item_type = list_item_type(array_field->type);
    if (ends_with_suffix(item_type, "Response"))
    {
        item_type = item_type.substr(0, item_type.size() - std::string_view{"Response"}.size());
    }
    return find_entity(ir, item_type);
}

std::vector<std::string> list_response_selector(
    const IrSystem& ir,
    const IrApi& api
)
{
    std::vector<std::string> selector;
    if (!api.output.has_value())
    {
        return selector;
    }
    const auto* shape = find_shape(ir, *api.output);
    const auto* array_field = shape == nullptr ? nullptr : list_response_array_field(*shape);
    if (shape == nullptr || array_field == nullptr)
    {
        return selector;
    }
    for (const auto& field : shape->fields)
    {
        if (&field != array_field)
        {
            selector.push_back(field.name);
        }
    }
    return selector;
}

std::int64_t duration_seconds(const std::optional<std::string>& value)
{
    if (!value.has_value() || value->empty() || (*value)[0] != 'P')
    {
        return 0;
    }

    std::int64_t total = 0;
    std::int64_t current = 0;
    bool in_time = false;
    for (std::size_t i = 1; i < value->size(); ++i)
    {
        const auto ch = (*value)[i];
        if (std::isdigit(static_cast<unsigned char>(ch)) != 0)
        {
            current = current * 10 + static_cast<std::int64_t>(ch - '0');
            continue;
        }
        if (ch == 'T')
        {
            in_time = true;
            continue;
        }
        if (ch == 'D')
        {
            total += current * 24 * 60 * 60;
        }
        else if (in_time && ch == 'H')
        {
            total += current * 60 * 60;
        }
        else if (in_time && ch == 'M')
        {
            total += current * 60;
        }
        else if (in_time && ch == 'S')
        {
            total += current;
        }
        current = 0;
    }
    return total;
}

std::vector<std::string> api_path_parameters(const IrApi& api)
{
    std::vector<std::string> parameters;
    if (!api.path.has_value())
    {
        return parameters;
    }
    const auto& path = *api.path;
    std::size_t offset = 0;
    while (offset < path.size())
    {
        const auto open = path.find('{', offset);
        if (open == std::string::npos)
        {
            break;
        }
        const auto close = path.find('}', open + 1);
        if (close == std::string::npos)
        {
            break;
        }
        if (close > open + 1)
        {
            parameters.push_back(path.substr(open + 1, close - open - 1));
        }
        offset = close + 1;
    }
    return parameters;
}

std::vector<std::string> index_backed_path_selector(
    const IrEntity& entity,
    const IrApi& api
)
{
    const auto path_parameters = api_path_parameters(api);
    const IrIndex* selected = nullptr;
    std::size_t selected_prefix_fields = 0;
    for (const auto& index : entity.indexes)
    {
        std::size_t prefix_fields = 0;
        for (const auto& field_name : index.fields)
        {
            if (std::find(path_parameters.begin(), path_parameters.end(), field_name) ==
                path_parameters.end())
            {
                break;
            }
            ++prefix_fields;
        }
        if (prefix_fields > selected_prefix_fields)
        {
            selected = &index;
            selected_prefix_fields = prefix_fields;
        }
    }
    if (selected == nullptr)
    {
        return {};
    }
    return std::vector<std::string>(
        selected->fields.begin(), selected->fields.begin() + selected_prefix_fields
    );
}

bool selector_has_backing_index(
    const IrEntity& entity,
    const std::vector<std::string>& selector
)
{
    if (selector.empty())
    {
        return false;
    }
    for (const auto& index : entity.indexes)
    {
        if (selector.size() <= index.fields.size() &&
            std::equal(selector.begin(), selector.end(), index.fields.begin()))
        {
            return true;
        }
    }
    return false;
}

std::string plural_pascal_identifier(const std::string& value)
{
    const auto singular = pascal_identifier(value, "Entities");
    if (!singular.empty() && singular.back() == 'y')
    {
        return singular.substr(0, singular.size() - 1) + "ies";
    }
    if (!singular.empty() && singular.back() == 's')
    {
        return singular + "es";
    }
    return singular + "s";
}

std::string status_path(const std::string& resource)
{
    if (!resource.empty() && resource.back() == '/')
    {
        return resource + "status";
    }
    return resource + "/status";
}

std::vector<std::string> list_selector_fields(
    const SemanticEntity& entity,
    const SemanticEntityApiList& list
)
{
    if (list.by.size() == 1)
    {
        for (const auto& index : entity.indexes)
        {
            if (index.name == list.by[0])
            {
                return index.fields;
            }
        }
    }
    return list.by;
}

void append_synthetic_api(
    IrSystem& ir,
    std::vector<std::string>& synthetic_api_names,
    IrApi api
)
{
    synthetic_api_names.push_back(api.name);
    if (!has_api(ir, api.name))
    {
        ir.apis.push_back(std::move(api));
    }
}

void append_entity_api_operations(
    const SemanticEntity& entity,
    IrSystem& ir,
    std::vector<std::string>& synthetic_api_names
)
{
    if (!entity.api.has_value())
    {
        return;
    }

    const auto entity_name = pascal_identifier(entity.name, "Entity");
    const auto entity_plural = plural_pascal_identifier(entity.name);
    const auto response_name = entity_name + "Response";
    const auto resource = entity.api->resource.value_or("");

    if (entity.api->create.has_value())
    {
        const auto create_name = entity.api->create->name.value_or("Create" + entity_name);
        append_synthetic_api(
            ir, synthetic_api_names,
            IrApi{
                create_name,
                std::string{"POST"},
                resource,
                create_name + "Request",
                response_name,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                entity.name,
                std::string{"create"},
                {},
            }
        );
    }

    if (entity.api->get.has_value())
    {
        const auto get_name = entity.api->get->name.value_or("Get" + entity_name);
        append_synthetic_api(
            ir, synthetic_api_names,
            IrApi{
                get_name,
                std::string{"GET"},
                resource,
                std::nullopt,
                response_name,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                entity.name,
                std::string{"get"},
                {},
            }
        );
    }

    for (const auto& list : entity.api->lists)
    {
        const auto list_name = list.name.value_or("List" + entity_plural);
        append_synthetic_api(
            ir, synthetic_api_names,
            IrApi{
                list_name,
                std::string{"GET"},
                list.path,
                std::nullopt,
                list_name + "Response",
                std::nullopt,
                std::nullopt,
                std::nullopt,
                entity.name,
                std::string{"list"},
                list_selector_fields(entity, list),
            }
        );
    }

    if (entity.api->update_status.has_value())
    {
        const auto update_name =
            entity.api->update_status->name.value_or("Update" + entity_name + "Status");
        append_synthetic_api(
            ir, synthetic_api_names,
            IrApi{
                update_name,
                std::string{"PATCH"},
                status_path(resource),
                update_name + "Request",
                response_name,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                entity.name,
                std::string{"update_status"},
                {},
            }
        );
    }

    if (entity.api->delete_.has_value())
    {
        const auto delete_name = entity.api->delete_->name.value_or("Delete" + entity_name);
        append_synthetic_api(
            ir, synthetic_api_names,
            IrApi{
                delete_name,
                std::string{"DELETE"},
                resource,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                entity.name,
                std::string{"delete"},
                {},
            }
        );
    }
}

void append_entity_api_server_memberships(
    IrSystem& ir,
    const std::vector<std::string>& synthetic_api_names
)
{
    if (synthetic_api_names.empty())
    {
        return;
    }

    if (ir.api_servers.empty())
    {
        ir.api_servers.push_back(IrApiServer{"EntityApi", synthetic_api_names, std::nullopt});
        return;
    }

    for (auto& api_server : ir.api_servers)
    {
        for (const auto& api_name : synthetic_api_names)
        {
            if (!has_served_api(api_server, api_name))
            {
                api_server.serves.push_back(api_name);
            }
        }
    }
}

void attach_explicit_crud_metadata(
    IrSystem& ir,
    IrApi& api
)
{
    if (api.entity.has_value() || api.repository_operation.has_value())
    {
        return;
    }

    if (api.method == "POST" && api.input.has_value())
    {
        if (const auto* entity = response_entity(ir, api.output); entity != nullptr)
        {
            api.entity = entity->name;
            api.repository_operation = "create";
        }
        return;
    }

    if (api.method == "GET")
    {
        if (const auto* entity = list_response_entity(ir, api); entity != nullptr)
        {
            api.entity = entity->name;
            api.repository_operation = "list";
            api.list_selector = index_backed_path_selector(*entity, api);
            if (api.list_selector.empty())
            {
                auto response_selector = list_response_selector(ir, api);
                if (selector_has_backing_index(*entity, response_selector))
                {
                    api.list_selector = std::move(response_selector);
                }
            }
            return;
        }
        if (const auto* entity = response_entity(ir, api.output); entity != nullptr)
        {
            api.entity = entity->name;
            api.repository_operation = "get";
        }
        return;
    }

    if (api.method == "PATCH" && api.input.has_value())
    {
        if (const auto* entity = response_entity(ir, api.output); entity != nullptr)
        {
            api.entity = entity->name;
            api.repository_operation = "update_status";
        }
        return;
    }

    if (api.method == "DELETE" && api.name.rfind("Delete", 0) == 0)
    {
        if (const auto* entity =
                find_entity(ir, api.name.substr(std::string_view{"Delete"}.size()));
            entity != nullptr)
        {
            api.entity = entity->name;
            api.repository_operation = "delete";
        }
    }
}

} // namespace

void lower_ir_runtime(
    const SemanticSystem& system,
    IrSystem& ir
)
{
    for (const auto& queue : system.queues)
    {
        IrQueue ir_queue;
        ir_queue.name = queue.name;
        ir_queue.namespace_name = queue.namespace_name;
        ir_queue.channel = queue.channel;
        ir_queue.visibility_timeout = queue.visibility_timeout;
        ir_queue.max_attempts = queue.max_attempts;
        ir_queue.dead_letter = reference_name(queue.dead_letter);
        for (const auto& message : queue.messages)
        {
            IrMessage ir_message;
            ir_message.name = message.name;
            ir_message.idempotency_key = message.idempotency_key;
            ir_message.payload_fields = lower_fields(message.payload_fields);
            ir_queue.messages.push_back(std::move(ir_message));
        }
        ir.queues.push_back(std::move(ir_queue));
    }

    for (const auto& lease : system.leases)
    {
        const auto ttl_seconds = duration_seconds(lease.ttl);
        ir.leases.push_back(
            IrLease{
                lease.name,
                lease.resource,
                lease.resource.value_or(lease.name),
                lease.ttl,
                ttl_seconds,
                lease.renew_every,
                lease.renew_every.has_value() ? duration_seconds(lease.renew_every) : ttl_seconds,
                lease.holder,
                lease.fencing_token,
                lease.fencing_token.value_or(false),
                lease.max_ttl,
                lease.max_ttl.has_value()
                    ? std::optional<std::int64_t>{duration_seconds(lease.max_ttl)}
                    : std::nullopt,
            }
        );
    }

    for (const auto& worker : system.workers)
    {
        ir.workers.push_back(
            IrWorker{
                worker.name,
                worker.singleton,
                reference_name(worker.lease),
                reference_name(worker.polls),
                reference_name(worker.executes),
                worker.concurrency,
            }
        );
    }

    for (const auto& api_server : system.api_servers)
    {
        ir.api_servers.push_back(
            IrApiServer{
                api_server.name,
                reference_names(api_server.serves),
                api_server.concurrency,
            }
        );
    }

    for (const auto& api : system.apis)
    {
        IrApi lowered{
            api.name,
            api.method,
            api.path,
            reference_name(api.input),
            reference_name(api.output),
            reference_name(api.error),
            reference_name(api.starts_workflow),
            reference_name(api.enqueues),
            std::nullopt,
            std::nullopt,
            {},
        };
        attach_explicit_crud_metadata(ir, lowered);
        ir.apis.push_back(std::move(lowered));
    }

    std::vector<std::string> synthetic_api_names;
    for (const auto& entity : system.entities)
    {
        append_entity_api_operations(entity, ir, synthetic_api_names);
    }
    append_entity_api_server_memberships(ir, synthetic_api_names);
}

} // namespace statespec
