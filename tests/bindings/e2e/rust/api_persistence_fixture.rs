use std::collections::BTreeMap;

use statespec_generated::api_dispatcher;
use statespec_generated::api_handler_registry::DefaultApiHandler;
use statespec_generated::backend::{Backend, BackendError, BackendResult};
use statespec_generated::descriptor_types::{ApiRequestContext, ApiResponse};
use statespec_generated::json::Json;
use statespec_generated::memory_backend::InMemoryBackend;

fn api_request(api_name: &str, method: &str, path: &str, body: Json) -> ApiRequestContext {
    ApiRequestContext {
        server_name: "EntityApi".to_string(),
        api_name: api_name.to_string(),
        method: Some(method.to_string()),
        path: Some(path.to_string()),
        body,
    }
}

fn object(values: Vec<(&str, Json)>) -> Json {
    Json::Object(
        values
            .into_iter()
            .map(|(key, value)| (key.to_string(), value))
            .collect(),
    )
}

fn require_status(response: &ApiResponse, status: i32) {
    assert_eq!(response.status_code, status);
}

fn require_string(body: &Json, field: &str, expected: &str) {
    let Json::Object(values) = body else {
        panic!("expected object body");
    };
    assert_eq!(
        values.get(field),
        Some(&Json::String(expected.to_string())),
        "unexpected string field {field}",
    );
}

fn require_array_not_empty(body: &Json, field: &str) {
    let Json::Object(values) = body else {
        panic!("expected object body");
    };
    let Some(Json::Array(items)) = values.get(field) else {
        panic!("expected array field {field}");
    };
    assert!(!items.is_empty(), "expected non-empty array field {field}");
}

fn require_array_size(body: &Json, field: &str, expected: usize) {
    let Json::Object(values) = body else {
        panic!("expected object body");
    };
    let Some(Json::Array(items)) = values.get(field) else {
        panic!("expected array field {field}");
    };
    assert_eq!(items.len(), expected, "unexpected array size for {field}");
}

fn call_api<B: Backend>(
    handler: &DefaultApiHandler<B>,
    request: ApiRequestContext,
) -> BackendResult<ApiResponse> {
    let route_name = format!("{}.{}", request.server_name, request.api_name);
    api_dispatcher::dispatch_api_route(
        &handler.backend,
        handler.business_handler.as_deref(),
        &route_name,
        &request,
    )?
    .ok_or_else(|| BackendError::Internal {
        message: format!("route not handled: {route_name}"),
    })
}

#[test]
fn generated_api_persistence_handlers_round_trip_entities() {
    let handler = DefaultApiHandler::new(InMemoryBackend::new());

    let account = call_api(
        &handler,
        api_request(
            "CreateAccount",
            "POST",
            "/v1/tenants/t1/accounts/a1",
            object(vec![("display_name", Json::String("Acme".to_string()))]),
        ),
    )
    .expect("create account failed");
    require_status(&account, 201);
    require_string(&account.body, "status", "Active");

    let project = call_api(
        &handler,
        api_request(
            "CreateProject",
            "POST",
            "/v1/tenants/t1/projects/p1",
            object(vec![
                ("account_id", Json::String("a1".to_string())),
                ("name", Json::String("Core".to_string())),
            ]),
        ),
    )
    .expect("create project failed");
    require_status(&project, 201);
    require_string(&project.body, "status", "Planned");

    let task = call_api(
        &handler,
        api_request(
            "CreateTask",
            "POST",
            "/v1/tenants/t1/tasks/t1",
            object(vec![
                ("account_id", Json::String("a1".to_string())),
                ("project_id", Json::String("p1".to_string())),
                ("title", Json::String("Ship".to_string())),
                ("priority", Json::Integer(7)),
            ]),
        ),
    )
    .expect("create task failed");
    require_status(&task, 201);
    require_string(&task.body, "status", "Open");

    let other_account = call_api(
        &handler,
        api_request(
            "CreateAccount",
            "POST",
            "/v1/tenants/t1/accounts/a2",
            object(vec![("display_name", Json::String("Other".to_string()))]),
        ),
    )
    .expect("create other account failed");
    require_status(&other_account, 201);

    let other_project = call_api(
        &handler,
        api_request(
            "CreateProject",
            "POST",
            "/v1/tenants/t1/projects/p2",
            object(vec![
                ("account_id", Json::String("a2".to_string())),
                ("name", Json::String("Other".to_string())),
            ]),
        ),
    )
    .expect("create other project failed");
    require_status(&other_project, 201);

    let other_task = call_api(
        &handler,
        api_request(
            "CreateTask",
            "POST",
            "/v1/tenants/t1/tasks/t2",
            object(vec![
                ("account_id", Json::String("a2".to_string())),
                ("project_id", Json::String("p2".to_string())),
                ("title", Json::String("Other".to_string())),
                ("priority", Json::Integer(2)),
            ]),
        ),
    )
    .expect("create other task failed");
    require_status(&other_task, 201);

    let got_account = call_api(
        &handler,
        api_request(
            "GetAccount",
            "GET",
            "/v1/tenants/t1/accounts/a1",
            Json::Object(BTreeMap::new()),
        ),
    )
    .expect("get account failed");
    require_status(&got_account, 200);
    require_string(&got_account.body, "display_name", "Acme");

    let got_project = call_api(
        &handler,
        api_request(
            "GetProject",
            "GET",
            "/v1/tenants/t1/projects/p1",
            Json::Object(BTreeMap::new()),
        ),
    )
    .expect("get project failed");
    require_status(&got_project, 200);
    require_string(&got_project.body, "name", "Core");

    let got_task = call_api(
        &handler,
        api_request(
            "GetTask",
            "GET",
            "/v1/tenants/t1/tasks/t1",
            Json::Object(BTreeMap::new()),
        ),
    )
    .expect("get task failed");
    require_status(&got_task, 200);
    require_string(&got_task.body, "title", "Ship");

    let missing_account = call_api(
        &handler,
        api_request(
            "GetAccount",
            "GET",
            "/v1/tenants/t1/accounts/missing",
            Json::Object(BTreeMap::new()),
        ),
    )
    .expect("get missing account failed");
    require_status(&missing_account, 404);

    let accounts = call_api(
        &handler,
        api_request(
            "ListAccounts",
            "GET",
            "/v1/tenants/t1/accounts",
            Json::Object(BTreeMap::new()),
        ),
    )
    .expect("list accounts failed");
    require_status(&accounts, 200);
    require_array_not_empty(&accounts.body, "accounts");
    require_array_size(&accounts.body, "accounts", 2);

    let projects = call_api(
        &handler,
        api_request(
            "ListAccountProjects",
            "GET",
            "/v1/tenants/t1/accounts/a1/projects",
            Json::Object(BTreeMap::new()),
        ),
    )
    .expect("list projects failed");
    require_status(&projects, 200);
    require_array_not_empty(&projects.body, "projects");
    require_array_size(&projects.body, "projects", 1);

    let tasks = call_api(
        &handler,
        api_request(
            "ListProjectTasks",
            "GET",
            "/v1/tenants/t1/projects/p1/tasks",
            Json::Object(BTreeMap::new()),
        ),
    )
    .expect("list tasks failed");
    require_status(&tasks, 200);
    require_array_not_empty(&tasks.body, "tasks");
    require_array_size(&tasks.body, "tasks", 1);

    let active_project = call_api(
        &handler,
        api_request(
            "UpdateProjectStatus",
            "PATCH",
            "/v1/tenants/t1/projects/p1/status",
            object(vec![("status", Json::String("Active".to_string()))]),
        ),
    )
    .expect("update project failed");
    require_status(&active_project, 200);
    require_string(&active_project.body, "status", "Active");

    let in_progress_task = call_api(
        &handler,
        api_request(
            "UpdateTaskStatus",
            "PATCH",
            "/v1/tenants/t1/tasks/t1/status",
            object(vec![("status", Json::String("InProgress".to_string()))]),
        ),
    )
    .expect("update task failed");
    require_status(&in_progress_task, 200);
    require_string(&in_progress_task.body, "status", "InProgress");

    let missing_project_status = call_api(
        &handler,
        api_request(
            "UpdateProjectStatus",
            "PATCH",
            "/v1/tenants/t1/projects/missing/status",
            object(vec![("status", Json::String("Active".to_string()))]),
        ),
    )
    .expect("update missing project failed");
    require_status(&missing_project_status, 404);

    let deleted_project = call_api(
        &handler,
        api_request(
            "DeleteProject",
            "DELETE",
            "/v1/tenants/t1/projects/p1",
            Json::Object(BTreeMap::new()),
        ),
    )
    .expect("delete project failed");
    require_status(&deleted_project, 204);

    let missing_project_delete = call_api(
        &handler,
        api_request(
            "DeleteProject",
            "DELETE",
            "/v1/tenants/t1/projects/missing",
            Json::Object(BTreeMap::new()),
        ),
    )
    .expect("delete missing project failed");
    require_status(&missing_project_delete, 404);

    let got_deleted_project = call_api(
        &handler,
        api_request(
            "GetProject",
            "GET",
            "/v1/tenants/t1/projects/p1",
            Json::Object(BTreeMap::new()),
        ),
    )
    .expect("get deleted project failed");
    require_status(&got_deleted_project, 200);
    require_string(&got_deleted_project.body, "status", "Deleted");
}
