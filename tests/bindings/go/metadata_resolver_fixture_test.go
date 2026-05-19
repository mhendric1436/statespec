package backend

import (
	"context"
	"testing"
)

type fixtureTx struct{}

func (fixtureTx) IsOpen() bool                { return true }
func (fixtureTx) Abort(context.Context) error { return nil }

type fixtureResolver struct {
	calls int
}

func (r *fixtureResolver) ResolveMetadata(context.Context, Backend, ExternalSystemMetadataLookup) (*ExternalSystemMetadataResolution, error) {
	return nil, nil
}

func (r *fixtureResolver) ResolveMetadataTx(ctx context.Context, tx Transaction, lookup ExternalSystemMetadataLookup) (*ExternalSystemMetadataResolution, error) {
	r.calls++
	document := JSONObject(map[string]JSON{
		"tenant_id": JSONString("tenant-a"),
		"base_url":  JSONString("https://api.stripe.test"),
	})
	return &ExternalSystemMetadataResolution{
		Record: VersionedRecord{
			Collection: CollectionName(lookup.MetadataEntity),
			Key:        Key("tenant-a/stripe/default"),
			Version:    Version(1),
			Document:   document,
		},
		MissingRequiredFields: MissingRequiredMetadataFields(document, lookup.RequiredFields),
	}, nil
}

type fixtureMappingApplicator struct{}

func (fixtureMappingApplicator) ApplyExternalSystemMetadataMappings(ctx context.Context, plan ExternalSystemMetadataMappingPlan, inputs ExternalSystemMetadataMappingInputs) (ExternalSystemMetadataMappingOutput, error) {
	output := ExternalSystemMetadataMappingOutput{
		ClientConfig:   map[string]JSON{},
		RequestPayload: map[string]JSON{},
		MissingSources: MissingExternalSystemMetadataMappingSources(plan, inputs),
	}
	for _, assignment := range plan.AllMappings {
		value, ok := inputs.AssignmentValue(assignment)
		if !ok {
			continue
		}
		if assignment.TargetRoot == "client" {
			output.ClientConfig[assignment.Field] = value
		} else if assignment.TargetRoot == "request" {
			output.RequestPayload[assignment.Field] = value
		}
	}
	return output, nil
}

type fixtureOperatorMetadataRepository struct{}

func (fixtureOperatorMetadataRepository) UpsertMetadataTx(ctx context.Context, tx Transaction, request ExternalSystemOperatorMetadataUpsertRequest) (*VersionedRecord, error) {
	version := Version(1)
	if request.ExpectedVersion != nil {
		version = *request.ExpectedVersion + 1
	}
	return &VersionedRecord{
		Collection: CollectionName(request.Lookup.MetadataEntity),
		Key:        Key("tenant-a/stripe/default"),
		Version:    version,
		Document:   request.Document,
	}, nil
}

func (fixtureOperatorMetadataRepository) GetMetadataTx(ctx context.Context, tx Transaction, request ExternalSystemOperatorMetadataGetRequest) (*VersionedRecord, error) {
	return &VersionedRecord{
		Collection: CollectionName(request.Lookup.MetadataEntity),
		Key:        Key("tenant-a/stripe/default"),
		Version:    Version(1),
		Document: JSONObject(map[string]JSON{
			"status": JSONString("Active"),
		}),
	}, nil
}

func (fixtureOperatorMetadataRepository) DisableMetadataTx(ctx context.Context, tx Transaction, request ExternalSystemOperatorMetadataDisableRequest) (*VersionedRecord, error) {
	version := Version(1)
	if request.ExpectedVersion != nil {
		version = *request.ExpectedVersion + 1
	}
	return &VersionedRecord{
		Collection: CollectionName(request.Lookup.MetadataEntity),
		Key:        Key("tenant-a/stripe/default"),
		Version:    version,
		Document: JSONObject(map[string]JSON{
			"status": JSONString(request.DisabledStatus),
		}),
	}, nil
}

func (fixtureOperatorMetadataRepository) DeleteMetadataTx(ctx context.Context, tx Transaction, request ExternalSystemOperatorMetadataDeleteRequest) (*VersionedRecord, error) {
	version := Version(1)
	if request.ExpectedVersion != nil {
		version = *request.ExpectedVersion + 1
	}
	return &VersionedRecord{
		Collection: CollectionName(request.Lookup.MetadataEntity),
		Key:        Key("tenant-a/stripe/default"),
		Version:    version,
		Document: JSONObject(map[string]JSON{
			"status": JSONString(request.DeletedStatus),
		}),
	}, nil
}

type fixtureOperatorMetadataAPIHandler struct{}

func (fixtureOperatorMetadataAPIHandler) HandleUpsertMetadataTx(ctx context.Context, tx Transaction, repository ExternalSystemOperatorMetadataRepository, request ExternalSystemOperatorMetadataUpsertRequest) (APIResponse, error) {
	record, err := repository.UpsertMetadataTx(ctx, tx, request)
	if err != nil {
		return APIResponse{}, err
	}
	status := 404
	if record != nil {
		status = 200
	}
	return APIResponse{StatusCode: status, Body: JSONObject(map[string]JSON{"operation": JSONString("upsert")})}, nil
}

func (fixtureOperatorMetadataAPIHandler) HandleGetMetadataTx(ctx context.Context, tx Transaction, repository ExternalSystemOperatorMetadataRepository, request ExternalSystemOperatorMetadataGetRequest) (APIResponse, error) {
	record, err := repository.GetMetadataTx(ctx, tx, request)
	if err != nil {
		return APIResponse{}, err
	}
	status := 404
	if record != nil {
		status = 200
	}
	return APIResponse{StatusCode: status, Body: JSONObject(map[string]JSON{"operation": JSONString("get")})}, nil
}

func (fixtureOperatorMetadataAPIHandler) HandleDisableMetadataTx(ctx context.Context, tx Transaction, repository ExternalSystemOperatorMetadataRepository, request ExternalSystemOperatorMetadataDisableRequest) (APIResponse, error) {
	record, err := repository.DisableMetadataTx(ctx, tx, request)
	if err != nil {
		return APIResponse{}, err
	}
	status := 404
	if record != nil {
		status = 200
	}
	return APIResponse{StatusCode: status, Body: JSONObject(map[string]JSON{"operation": JSONString("disable")})}, nil
}

func (fixtureOperatorMetadataAPIHandler) HandleDeleteMetadataTx(ctx context.Context, tx Transaction, repository ExternalSystemOperatorMetadataRepository, request ExternalSystemOperatorMetadataDeleteRequest) (APIResponse, error) {
	record, err := repository.DeleteMetadataTx(ctx, tx, request)
	if err != nil {
		return APIResponse{}, err
	}
	status := 404
	if record != nil {
		status = 200
	}
	return APIResponse{StatusCode: status, Body: JSONObject(map[string]JSON{"operation": JSONString("delete")})}, nil
}

func TestGeneratedMetadataResolverFixture(t *testing.T) {
	ctx := context.Background()
	resolver := &fixtureResolver{}
	tx := fixtureTx{}
	plan := BuildExternalSystemMetadataMappingPlan(ExternalSystemDescriptors()[0])
	if len(plan.AllMappings) != 4 || len(plan.ClientMappings) != 3 || len(plan.RequestMappings) != 1 || plan.ClientMappings[0].SourceRoot != "metadata" || plan.ClientMappings[0].SourceField != "base_url" || plan.ClientMappings[0].TargetRoot != "client" || plan.ClientMappings[0].Field != "base_url" || plan.RequestMappings[0].SourceRoot != "input" || plan.RequestMappings[0].SourceField != "order_id" || plan.RequestMappings[0].TargetRoot != "request" || plan.RequestMappings[0].Field != "order_id" {
		t.Fatalf("unexpected metadata mapping plan: %#v", plan)
	}
	var applicator ExternalSystemMetadataMappingApplicator = fixtureMappingApplicator{}
	mapped, err := applicator.ApplyExternalSystemMetadataMappings(ctx, plan, ExternalSystemMetadataMappingInputs{
		Input: map[string]JSON{
			"order_id": JSONString("order-1"),
		},
		Metadata: map[string]JSON{
			"base_url":   JSONString("https://api.stripe.test"),
			"auth_ref":   JSONString("secret:stripe"),
			"timeout_ms": JSONInt(5000),
		},
	})
	if err != nil || len(mapped.ClientConfig) != 3 || len(mapped.RequestPayload) != 1 || len(mapped.MissingSources) != 0 {
		t.Fatalf("unexpected mapped metadata output: %#v err=%v", mapped, err)
	}
	missingMapped, err := applicator.ApplyExternalSystemMetadataMappings(ctx, plan, ExternalSystemMetadataMappingInputs{
		Input: map[string]JSON{
			"order_id": JSONString("order-1"),
		},
		Metadata: map[string]JSON{
			"base_url": JSONString("https://api.stripe.test"),
			"auth_ref": JSONString("secret:stripe"),
		},
	})
	if err != nil || len(missingMapped.MissingSources) != 1 || missingMapped.MissingSources[0].Source != "metadata.timeout_ms" || missingMapped.MissingSources[0].TargetRoot != "client" || missingMapped.MissingSources[0].Field != "timeout_ms" {
		t.Fatalf("unexpected missing mapping diagnostics: %#v err=%v", missingMapped, err)
	}
	keys := []ExternalSystemMetadataKeyValue{
		{Field: "tenant_id", Value: JSONString("tenant-a")},
		{Field: "external_system_id", Value: JSONString("stripe")},
		{Field: "profile", Value: JSONString("default")},
	}
	lookup, ok := ExternalSystemMetadataLookupByName("Billing.Stripe", keys)
	if !ok || lookup == nil {
		t.Fatalf("expected metadata lookup")
	}
	version := Version(1)
	repository := ExternalSystemOperatorMetadataRepository(fixtureOperatorMetadataRepository{})
	upserted, err := repository.UpsertMetadataTx(ctx, tx, ExternalSystemOperatorMetadataUpsertRequest{
		Lookup:          *lookup,
		Document:        JSONObject(map[string]JSON{"tenant_id": JSONString("tenant-a")}),
		ExpectedVersion: &version,
	})
	if err != nil || upserted == nil || upserted.Version != 2 {
		t.Fatalf("unexpected metadata upsert result: %#v err=%v", upserted, err)
	}
	loaded, err := repository.GetMetadataTx(ctx, tx, ExternalSystemOperatorMetadataGetRequest{Lookup: *lookup})
	disabled, err2 := repository.DisableMetadataTx(ctx, tx, ExternalSystemOperatorMetadataDisableRequest{Lookup: *lookup, ExpectedVersion: &upserted.Version, DisabledStatus: "Disabled"})
	deleted, err3 := repository.DeleteMetadataTx(ctx, tx, ExternalSystemOperatorMetadataDeleteRequest{Lookup: *lookup, ExpectedVersion: &disabled.Version, DeletedStatus: "Deleted"})
	if err != nil || err2 != nil || err3 != nil || loaded == nil || disabled == nil || disabled.Version != 3 || deleted == nil || deleted.Version != 4 {
		t.Fatalf("unexpected metadata repository results loaded=%#v disabled=%#v deleted=%#v errs=%v/%v/%v", loaded, disabled, deleted, err, err2, err3)
	}
	apiHandler := ExternalSystemOperatorMetadataAPIHandler(fixtureOperatorMetadataAPIHandler{})
	apiResponse, err := apiHandler.HandleUpsertMetadataTx(ctx, tx, repository, ExternalSystemOperatorMetadataUpsertRequest{
		Lookup:          *lookup,
		Document:        JSONObject(map[string]JSON{"tenant_id": JSONString("tenant-a")}),
		ExpectedVersion: &deleted.Version,
	})
	if err != nil || apiResponse.StatusCode != 200 {
		t.Fatalf("unexpected operator metadata API response: %#v err=%v", apiResponse, err)
	}
	resolved, ok, err := ResolveExternalSystemMetadataByNameTx(ctx, resolver, tx, "Billing.Stripe", keys)
	if err != nil || !ok || resolved == nil || resolved.Complete() || resolver.calls != 1 {
		t.Fatalf("expected incomplete metadata resolution through resolver, ok=%v resolved=%#v err=%v calls=%d", ok, resolved, err, resolver.calls)
	}

	keys = keys[:len(keys)-1]
	skipped, ok, err := ResolveExternalSystemMetadataByNameTx(ctx, resolver, tx, "Billing.Stripe", keys)
	if err != nil || !ok || skipped != nil || resolver.calls != 1 {
		t.Fatalf("expected incomplete key to skip resolver, ok=%v skipped=%#v err=%v calls=%d", ok, skipped, err, resolver.calls)
	}
}
