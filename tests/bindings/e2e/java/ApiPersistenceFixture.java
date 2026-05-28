package com.statespec.generated;

import com.statespec.backend.Json;
import com.statespec.backend.memory.InMemoryBackend;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public final class ApiPersistenceFixture
{
    private ApiPersistenceFixture() {}

    public static void main(String[] args) throws Exception
    {
        ApiHandlers.Handler handler = ApiHandlerRegistry.defaultHandler(new InMemoryBackend());

        ApiResponse account = handler.handleCreateAccount(request(
            "CreateAccount", "POST", "/v1/tenants/t1/accounts/a1",
            Json.object(Map.of("display_name", Json.string("Acme")))
        ));
        requireStatus(account, 201);
        requireString(account.body(), "status", "Active");

        ApiResponse project = handler.handleCreateProject(request(
            "CreateProject", "POST", "/v1/tenants/t1/projects/p1",
            Json.object(Map.of("account_id", Json.string("a1"), "name", Json.string("Core")))
        ));
        requireStatus(project, 201);
        requireString(project.body(), "status", "Planned");

        ApiResponse task = handler.handleCreateTask(request(
            "CreateTask", "POST", "/v1/tenants/t1/tasks/t1",
            Json.object(Map.of(
                "account_id", Json.string("a1"), "project_id", Json.string("p1"), "title",
                Json.string("Ship"), "priority", Json.integer(7)
            ))
        ));
        requireStatus(task, 201);
        requireString(task.body(), "status", "Open");

        ApiResponse otherAccount = handler.handleCreateAccount(request(
            "CreateAccount", "POST", "/v1/tenants/t1/accounts/a2",
            Json.object(Map.of("display_name", Json.string("Other")))
        ));
        requireStatus(otherAccount, 201);

        ApiResponse otherProject = handler.handleCreateProject(request(
            "CreateProject", "POST", "/v1/tenants/t1/projects/p2",
            Json.object(Map.of("account_id", Json.string("a2"), "name", Json.string("Other")))
        ));
        requireStatus(otherProject, 201);

        ApiResponse otherTask = handler.handleCreateTask(request(
            "CreateTask", "POST", "/v1/tenants/t1/tasks/t2",
            Json.object(Map.of(
                "account_id", Json.string("a2"), "project_id", Json.string("p2"), "title",
                Json.string("Other"), "priority", Json.integer(2)
            ))
        ));
        requireStatus(otherTask, 201);

        ApiResponse gotAccount =
            handler.handleGetAccount(request("GetAccount", "GET", "/v1/tenants/t1/accounts/a1"));
        requireStatus(gotAccount, 200);
        requireString(gotAccount.body(), "display_name", "Acme");

        ApiResponse gotProject =
            handler.handleGetProject(request("GetProject", "GET", "/v1/tenants/t1/projects/p1"));
        requireStatus(gotProject, 200);
        requireString(gotProject.body(), "name", "Core");

        ApiResponse gotTask =
            handler.handleGetTask(request("GetTask", "GET", "/v1/tenants/t1/tasks/t1"));
        requireStatus(gotTask, 200);
        requireString(gotTask.body(), "title", "Ship");

        ApiResponse missingAccount = handler.handleGetAccount(
            request("GetAccount", "GET", "/v1/tenants/t1/accounts/missing")
        );
        requireStatus(missingAccount, 404);

        ApiResponse accounts =
            handler.handleListAccounts(request("ListAccounts", "GET", "/v1/tenants/t1/accounts"));
        requireStatus(accounts, 200);
        requireArrayNotEmpty(accounts.body(), "accounts");
        requireArraySize(accounts.body(), "accounts", 2);

        ApiResponse projects = handler.handleListAccountProjects(
            request("ListAccountProjects", "GET", "/v1/tenants/t1/accounts/a1/projects")
        );
        requireStatus(projects, 200);
        requireArrayNotEmpty(projects.body(), "projects");
        requireArraySize(projects.body(), "projects", 1);

        ApiResponse tasks = handler.handleListProjectTasks(
            request("ListProjectTasks", "GET", "/v1/tenants/t1/projects/p1/tasks")
        );
        requireStatus(tasks, 200);
        requireArrayNotEmpty(tasks.body(), "tasks");
        requireArraySize(tasks.body(), "tasks", 1);

        ApiResponse activeProject = handler.handleUpdateProjectStatus(request(
            "UpdateProjectStatus", "PATCH", "/v1/tenants/t1/projects/p1/status",
            Json.object(Map.of("status", Json.string("Active")))
        ));
        requireStatus(activeProject, 200);
        requireString(activeProject.body(), "status", "Active");

        ApiResponse inProgressTask = handler.handleUpdateTaskStatus(request(
            "UpdateTaskStatus", "PATCH", "/v1/tenants/t1/tasks/t1/status",
            Json.object(Map.of("status", Json.string("InProgress")))
        ));
        requireStatus(inProgressTask, 200);
        requireString(inProgressTask.body(), "status", "InProgress");

        ApiResponse missingProjectStatus = handler.handleUpdateProjectStatus(request(
            "UpdateProjectStatus", "PATCH", "/v1/tenants/t1/projects/missing/status",
            Json.object(Map.of("status", Json.string("Active")))
        ));
        requireStatus(missingProjectStatus, 404);

        ApiResponse deletedProject = handler.handleDeleteProject(
            request("DeleteProject", "DELETE", "/v1/tenants/t1/projects/p1")
        );
        requireStatus(deletedProject, 204);

        ApiResponse missingProjectDelete = handler.handleDeleteProject(
            request("DeleteProject", "DELETE", "/v1/tenants/t1/projects/missing")
        );
        requireStatus(missingProjectDelete, 404);

        ApiResponse gotDeletedProject =
            handler.handleGetProject(request("GetProject", "GET", "/v1/tenants/t1/projects/p1"));
        requireStatus(gotDeletedProject, 200);
        requireString(gotDeletedProject.body(), "status", "Deleted");
    }

    private static ApiRequestContext request(
        String apiName,
        String method,
        String path
    )
    {
        return request(apiName, method, path, Json.nullValue());
    }

    private static ApiRequestContext request(
        String apiName,
        String method,
        String path,
        Json body
    )
    {
        return new ApiRequestContext(
            "EntityApi", apiName, Optional.of(method), Optional.of(path), body
        );
    }

    private static void requireStatus(
        ApiResponse response,
        int status
    )
    {
        if (response.statusCode() != status)
        {
            throw new IllegalStateException("unexpected status code");
        }
    }

    private static void requireString(
        Json body,
        String field,
        String expected
    )
    {
        Json value = body.find(field).orElseThrow();
        if (!(value instanceof Json.StringValue stringValue) ||
            !stringValue.value().equals(expected))
        {
            throw new IllegalStateException("unexpected string field " + field);
        }
    }

    private static void requireArrayNotEmpty(
        Json body,
        String field
    )
    {
        Json value = body.find(field).orElseThrow();
        if (!(value instanceof Json.ArrayValue arrayValue) || arrayValue.values().isEmpty())
        {
            throw new IllegalStateException("expected non-empty array field " + field);
        }
    }

    private static void requireArraySize(
        Json body,
        String field,
        int expected
    )
    {
        Json value = body.find(field).orElseThrow();
        if (!(value instanceof Json.ArrayValue arrayValue) ||
            arrayValue.values().size() != expected)
        {
            throw new IllegalStateException("unexpected array size for " + field);
        }
    }
}
