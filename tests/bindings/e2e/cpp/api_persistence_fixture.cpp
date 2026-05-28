#include "api/api_handler_registry.hpp"
#include "common/memory/backend.hpp"

#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{

using statespec::backend::Json;
using statespec_generated::api::ApiRequestContext;
using statespec_generated::api::ApiResponse;

ApiRequestContext request(
    std::string api_name,
    std::string method,
    std::string path,
    Json body = Json::object({})
)
{
    return ApiRequestContext{"EntityApi", std::move(api_name), method, std::move(path), body};
}

const Json& member(
    const Json& value,
    const std::string& name
)
{
    const auto* found = value.find(name);
    if (found == nullptr)
    {
        throw std::runtime_error("missing JSON member: " + name);
    }
    return *found;
}

void require_status(
    const ApiResponse& response,
    int status
)
{
    static int check = 0;
    ++check;
    if (response.status_code != status)
    {
        std::ostringstream message;
        message << "unexpected status code at check " << check << ": expected " << status << " got "
                << response.status_code;
        throw std::runtime_error(message.str());
    }
}

void require_string(
    const Json& value,
    const std::string& expected
)
{
    if (!value.is_string() || value.as_string() != expected)
    {
        throw std::runtime_error("unexpected string value");
    }
}

void require_non_empty_array(const Json& value)
{
    if (!value.is_array() || value.as_array().empty())
    {
        throw std::runtime_error("expected non-empty array");
    }
}

void require_array_size(
    const Json& value,
    std::size_t expected
)
{
    if (!value.is_array() || value.as_array().size() != expected)
    {
        throw std::runtime_error("unexpected array size");
    }
}

} // namespace

int main()
{
    statespec::backend::memory::InMemoryBackend backend;
    statespec_generated::api::DefaultApiHandler handler{backend};

    const auto account = handler.handle_create_account(request(
        "CreateAccount", "POST", "/v1/tenants/t1/accounts/a1",
        Json::object({{"display_name", "Acme"}})
    ));
    require_status(account, 201);
    require_string(member(account.body, "status"), "Active");

    const auto project = handler.handle_create_project(request(
        "CreateProject", "POST", "/v1/tenants/t1/projects/p1",
        Json::object({{"account_id", "a1"}, {"name", "Core"}})
    ));
    require_status(project, 201);
    require_string(member(project.body, "status"), "Planned");

    const auto task = handler.handle_create_task(request(
        "CreateTask", "POST", "/v1/tenants/t1/tasks/t1",
        Json::object(
            {{"account_id", "a1"}, {"project_id", "p1"}, {"title", "Ship"}, {"priority", 7}}
        )
    ));
    require_status(task, 201);
    require_string(member(task.body, "status"), "Open");

    const auto other_account = handler.handle_create_account(request(
        "CreateAccount", "POST", "/v1/tenants/t1/accounts/a2",
        Json::object({{"display_name", "Other"}})
    ));
    require_status(other_account, 201);

    const auto other_project = handler.handle_create_project(request(
        "CreateProject", "POST", "/v1/tenants/t1/projects/p2",
        Json::object({{"account_id", "a2"}, {"name", "Other"}})
    ));
    require_status(other_project, 201);

    const auto other_task = handler.handle_create_task(request(
        "CreateTask", "POST", "/v1/tenants/t1/tasks/t2",
        Json::object(
            {{"account_id", "a2"}, {"project_id", "p2"}, {"title", "Other"}, {"priority", 2}}
        )
    ));
    require_status(other_task, 201);

    const auto got_account =
        handler.handle_get_account(request("GetAccount", "GET", "/v1/tenants/t1/accounts/a1"));
    require_status(got_account, 200);
    require_string(member(got_account.body, "display_name"), "Acme");

    const auto got_project =
        handler.handle_get_project(request("GetProject", "GET", "/v1/tenants/t1/projects/p1"));
    require_status(got_project, 200);
    require_string(member(got_project.body, "name"), "Core");

    const auto got_task =
        handler.handle_get_task(request("GetTask", "GET", "/v1/tenants/t1/tasks/t1"));
    require_status(got_task, 200);
    require_string(member(got_task.body, "title"), "Ship");

    const auto accounts =
        handler.handle_list_accounts(request("ListAccounts", "GET", "/v1/tenants/t1/accounts"));
    require_status(accounts, 200);
    require_non_empty_array(member(accounts.body, "accounts"));
    require_array_size(member(accounts.body, "accounts"), 2);

    const auto projects = handler.handle_list_account_projects(
        request("ListAccountProjects", "GET", "/v1/tenants/t1/accounts/a1/projects")
    );
    require_status(projects, 200);
    require_non_empty_array(member(projects.body, "projects"));
    require_array_size(member(projects.body, "projects"), 1);

    const auto tasks = handler.handle_list_project_tasks(
        request("ListProjectTasks", "GET", "/v1/tenants/t1/projects/p1/tasks")
    );
    require_status(tasks, 200);
    require_non_empty_array(member(tasks.body, "tasks"));
    require_array_size(member(tasks.body, "tasks"), 1);

    const auto active_project = handler.handle_update_project_status(request(
        "UpdateProjectStatus", "PATCH", "/v1/tenants/t1/projects/p1/status",
        Json::object({{"status", "Active"}})
    ));
    require_status(active_project, 200);
    require_string(member(active_project.body, "status"), "Active");

    const auto in_progress_task = handler.handle_update_task_status(request(
        "UpdateTaskStatus", "PATCH", "/v1/tenants/t1/tasks/t1/status",
        Json::object({{"status", "InProgress"}})
    ));
    require_status(in_progress_task, 200);
    require_string(member(in_progress_task.body, "status"), "InProgress");

    const auto deleted_project = handler.handle_delete_project(
        request("DeleteProject", "DELETE", "/v1/tenants/t1/projects/p1")
    );
    require_status(deleted_project, 204);

    const auto got_deleted_project =
        handler.handle_get_project(request("GetProject", "GET", "/v1/tenants/t1/projects/p1"));
    require_status(got_deleted_project, 200);
    require_string(member(got_deleted_project.body, "status"), "Deleted");
}
