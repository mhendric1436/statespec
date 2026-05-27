#include "ir_runtime.hpp"

#include "identifier_case.hpp"
#include "ir_lowering_helpers.hpp"

#include <algorithm>
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
        ir.leases.push_back(
            IrLease{
                lease.name,
                lease.resource,
                lease.ttl,
                lease.renew_every,
                lease.holder,
                lease.fencing_token,
                lease.max_ttl,
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
        ir.apis.push_back(
            IrApi{
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
            }
        );
    }

    std::vector<std::string> synthetic_api_names;
    for (const auto& entity : system.entities)
    {
        append_entity_api_operations(entity, ir, synthetic_api_names);
    }
    append_entity_api_server_memberships(ir, synthetic_api_names);
}

} // namespace statespec
