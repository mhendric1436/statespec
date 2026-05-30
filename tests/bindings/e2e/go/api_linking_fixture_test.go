package backend_test

import (
	"context"
	"testing"

	api "statespec-generated/api/backend"
	common "statespec-generated/common/backend"
	"statespec-generated/common/backend/memory"
)

type linkingHandler struct {
	backend *memory.Backend
}

func (h linkingHandler) HandleStartProvision(ctx context.Context, request common.APIRequestContext) (common.APIResponse, error) {
	return h.recordRequest(ctx, request)
}

func (h linkingHandler) HandleReportProvisionReady(ctx context.Context, request common.APIRequestContext) (common.APIResponse, error) {
	return h.recordRequest(ctx, request)
}

func (h linkingHandler) recordRequest(ctx context.Context, request common.APIRequestContext) (common.APIResponse, error) {
	tx, err := h.backend.Begin(ctx)
	if err != nil {
		return common.APIResponse{}, err
	}
	if err := h.backend.Put(ctx, tx, common.CollectionName("api_requests"), common.Key("request-1"), common.JSONObject(map[string]common.JSON{
		"server": common.JSONString(request.ServerName),
		"api":    common.JSONString(request.APIName),
	})); err != nil {
		return common.APIResponse{}, err
	}
	if err := h.backend.Commit(ctx, tx); err != nil {
		return common.APIResponse{}, err
	}
	return common.APIResponse{
		StatusCode: 202,
		Body: common.JSONObject(map[string]common.JSON{
			"linked": common.JSONBool(true),
		}),
	}, nil
}

func TestGeneratedAPIServerLinksWithMemoryBackend(t *testing.T) {
	descriptor, ok := api.FindAPITierServer("ProvisionApi")
	if !ok {
		t.Fatal("ProvisionApi descriptor not found")
	}

	backend := memory.NewBackend()
	server := api.APITierServer{
		Descriptor: descriptor,
		Handler: api.DefaultAPITierHandler{
			Backend:         backend,
			BusinessHandler: linkingHandler{backend: backend},
		},
	}
	response, handled, err := server.Handle(context.Background(), "ProvisionApi.StartProvision", common.APIRequestContext{
		ServerName: "ProvisionApi",
		APIName:    "StartProvision",
		Method:     stringPtr("POST"),
		Path:       stringPtr("/v1/provision"),
		Body: common.JSONObject(map[string]common.JSON{
			"tenant_id": common.JSONString("tenant-1"),
		}),
	})
	if err != nil {
		t.Fatal(err)
	}
	if !handled || response.StatusCode != 202 {
		t.Fatalf("unexpected response: handled=%v response=%#v", handled, response)
	}
}

func stringPtr(value string) *string {
	return &value
}
