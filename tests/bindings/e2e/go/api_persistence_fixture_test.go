package backend_test

import (
	"context"
	"testing"

	api "statespec-generated/api/backend"
	common "statespec-generated/common/backend"
	"statespec-generated/common/backend/memory"
)

func TestGeneratedAPIPersistenceHandlers(t *testing.T) {
	ctx := context.Background()
	backend := memory.NewBackend()
	handler := api.DefaultAPITierHandler{Backend: backend}

	account, err := callAPI(ctx, handler, apiRequest("CreateAccount", "POST", "/v1/tenants/t1/accounts/a1", common.JSONObject(map[string]common.JSON{
		"display_name": common.JSONString("Acme"),
	})))
	requireResponse(t, account, err, 201)
	requireJSONString(t, account.Body, "status", "Active")

	project, err := callAPI(ctx, handler, apiRequest("CreateProject", "POST", "/v1/tenants/t1/projects/p1", common.JSONObject(map[string]common.JSON{
		"account_id": common.JSONString("a1"),
		"name":       common.JSONString("Core"),
	})))
	requireResponse(t, project, err, 201)
	requireJSONString(t, project.Body, "status", "Planned")

	task, err := callAPI(ctx, handler, apiRequest("CreateTask", "POST", "/v1/tenants/t1/tasks/t1", common.JSONObject(map[string]common.JSON{
		"account_id": common.JSONString("a1"),
		"project_id": common.JSONString("p1"),
		"title":      common.JSONString("Ship"),
		"priority":   common.JSONInt(7),
	})))
	requireResponse(t, task, err, 201)
	requireJSONString(t, task.Body, "status", "Open")

	otherAccount, err := callAPI(ctx, handler, apiRequest("CreateAccount", "POST", "/v1/tenants/t1/accounts/a2", common.JSONObject(map[string]common.JSON{
		"display_name": common.JSONString("Other"),
	})))
	requireResponse(t, otherAccount, err, 201)

	otherProject, err := callAPI(ctx, handler, apiRequest("CreateProject", "POST", "/v1/tenants/t1/projects/p2", common.JSONObject(map[string]common.JSON{
		"account_id": common.JSONString("a2"),
		"name":       common.JSONString("Other"),
	})))
	requireResponse(t, otherProject, err, 201)

	otherTask, err := callAPI(ctx, handler, apiRequest("CreateTask", "POST", "/v1/tenants/t1/tasks/t2", common.JSONObject(map[string]common.JSON{
		"account_id": common.JSONString("a2"),
		"project_id": common.JSONString("p2"),
		"title":      common.JSONString("Other"),
		"priority":   common.JSONInt(2),
	})))
	requireResponse(t, otherTask, err, 201)

	gotAccount, err := callAPI(ctx, handler, apiRequest("GetAccount", "GET", "/v1/tenants/t1/accounts/a1", common.JSONNull()))
	requireResponse(t, gotAccount, err, 200)
	requireJSONString(t, gotAccount.Body, "display_name", "Acme")

	gotProject, err := callAPI(ctx, handler, apiRequest("GetProject", "GET", "/v1/tenants/t1/projects/p1", common.JSONNull()))
	requireResponse(t, gotProject, err, 200)
	requireJSONString(t, gotProject.Body, "name", "Core")

	gotTask, err := callAPI(ctx, handler, apiRequest("GetTask", "GET", "/v1/tenants/t1/tasks/t1", common.JSONNull()))
	requireResponse(t, gotTask, err, 200)
	requireJSONString(t, gotTask.Body, "title", "Ship")

	missingAccount, err := callAPI(ctx, handler, apiRequest("GetAccount", "GET", "/v1/tenants/t1/accounts/missing", common.JSONNull()))
	requireResponse(t, missingAccount, err, 404)

	accounts, err := callAPI(ctx, handler, apiRequest("ListAccounts", "GET", "/v1/tenants/t1/accounts", common.JSONNull()))
	requireResponse(t, accounts, err, 200)
	requireJSONArrayNotEmpty(t, accounts.Body, "accounts")
	requireJSONArraySize(t, accounts.Body, "accounts", 2)

	projects, err := callAPI(ctx, handler, apiRequest("ListAccountProjects", "GET", "/v1/tenants/t1/accounts/a1/projects", common.JSONNull()))
	requireResponse(t, projects, err, 200)
	requireJSONArrayNotEmpty(t, projects.Body, "projects")
	requireJSONArraySize(t, projects.Body, "projects", 1)

	tasks, err := callAPI(ctx, handler, apiRequest("ListProjectTasks", "GET", "/v1/tenants/t1/projects/p1/tasks", common.JSONNull()))
	requireResponse(t, tasks, err, 200)
	requireJSONArrayNotEmpty(t, tasks.Body, "tasks")
	requireJSONArraySize(t, tasks.Body, "tasks", 1)

	activeProject, err := callAPI(ctx, handler, apiRequest("UpdateProjectStatus", "PATCH", "/v1/tenants/t1/projects/p1/status", common.JSONObject(map[string]common.JSON{
		"status": common.JSONString("Active"),
	})))
	requireResponse(t, activeProject, err, 200)
	requireJSONString(t, activeProject.Body, "status", "Active")

	inProgressTask, err := callAPI(ctx, handler, apiRequest("UpdateTaskStatus", "PATCH", "/v1/tenants/t1/tasks/t1/status", common.JSONObject(map[string]common.JSON{
		"status": common.JSONString("InProgress"),
	})))
	requireResponse(t, inProgressTask, err, 200)
	requireJSONString(t, inProgressTask.Body, "status", "InProgress")

	missingProjectStatus, err := callAPI(ctx, handler, apiRequest("UpdateProjectStatus", "PATCH", "/v1/tenants/t1/projects/missing/status", common.JSONObject(map[string]common.JSON{
		"status": common.JSONString("Active"),
	})))
	requireResponse(t, missingProjectStatus, err, 404)

	deletedProject, err := callAPI(ctx, handler, apiRequest("DeleteProject", "DELETE", "/v1/tenants/t1/projects/p1", common.JSONNull()))
	requireResponse(t, deletedProject, err, 204)

	missingProjectDelete, err := callAPI(ctx, handler, apiRequest("DeleteProject", "DELETE", "/v1/tenants/t1/projects/missing", common.JSONNull()))
	requireResponse(t, missingProjectDelete, err, 404)

	gotDeletedProject, err := callAPI(ctx, handler, apiRequest("GetProject", "GET", "/v1/tenants/t1/projects/p1", common.JSONNull()))
	requireResponse(t, gotDeletedProject, err, 200)
	requireJSONString(t, gotDeletedProject.Body, "status", "Deleted")
}

func apiRequest(apiName, method, path string, body common.JSON) common.APIRequestContext {
	return common.APIRequestContext{
		ServerName: "EntityApi",
		APIName:    apiName,
		Method:     apiPersistenceStringPtr(method),
		Path:       apiPersistenceStringPtr(path),
		Body:       body,
	}
}

func callAPI(ctx context.Context, handler api.DefaultAPITierHandler, request common.APIRequestContext) (common.APIResponse, error) {
	response, handled, err := api.DispatchAPITierRoute(
		ctx,
		handler.Backend,
		handler.BusinessHandler,
		request.ServerName+"."+request.APIName,
		request,
	)
	if err != nil {
		return common.APIResponse{}, err
	}
	if !handled {
		return common.APIResponse{StatusCode: 404, Body: common.JSONObject(map[string]common.JSON{})}, nil
	}
	return response, nil
}

func apiPersistenceStringPtr(value string) *string {
	return &value
}

func requireResponse(t *testing.T, response common.APIResponse, err error, status int) {
	t.Helper()
	if err != nil {
		t.Fatal(err)
	}
	if response.StatusCode != status {
		t.Fatalf("unexpected status code: got %d want %d", response.StatusCode, status)
	}
}

func requireJSONString(t *testing.T, body common.JSON, field string, expected string) {
	t.Helper()
	value, ok := body.Find(field)
	if !ok {
		t.Fatalf("missing field %s in %s", field, body.CanonicalString())
	}
	actual, ok := value.AsString()
	if !ok || actual != expected {
		t.Fatalf("unexpected %s: got %s want %s", field, value.CanonicalString(), expected)
	}
}

func requireJSONArrayNotEmpty(t *testing.T, body common.JSON, field string) {
	t.Helper()
	value, ok := body.Find(field)
	if !ok {
		t.Fatalf("missing field %s in %s", field, body.CanonicalString())
	}
	items, ok := value.AsArray()
	if !ok || len(items) == 0 {
		t.Fatalf("expected non-empty array field %s", field)
	}
}

func requireJSONArraySize(t *testing.T, body common.JSON, field string, expected int) {
	t.Helper()
	value, ok := body.Find(field)
	if !ok {
		t.Fatalf("missing field %s in %s", field, body.CanonicalString())
	}
	items, ok := value.AsArray()
	if !ok || len(items) != expected {
		t.Fatalf("unexpected array size for %s: got %d want %d", field, len(items), expected)
	}
}
