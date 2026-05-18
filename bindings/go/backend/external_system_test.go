package backend

import "testing"

func TestExternalSystemMetadataLookupContracts(t *testing.T) {
	tenantField := "tenant_id"
	lookup := ExternalSystemMetadataLookup{
		ExternalSystem: "Billing.Stripe",
		MetadataEntity: "ExternalSystemEndpoint",
		TenantField:    &tenantField,
		ProfileField:   "profile",
		KeyFields:      []string{"tenant_id", "external_system_id", "profile"},
		KeyValues: []ExternalSystemMetadataKeyValue{
			{Field: "tenant_id", Value: JSONString("tenant-a")},
			{Field: "external_system_id", Value: JSONString("stripe")},
			{Field: "profile", Value: JSONString("default")},
		},
		RequiredFields: []string{"base_url", "auth_ref", "timeout_ms"},
	}

	document := JSONObject(map[string]JSON{
		"tenant_id": JSONString("tenant-a"),
		"base_url":  JSONString("https://api.stripe.test"),
	})
	missing := MissingRequiredMetadataFields(document, lookup.RequiredFields)
	if lookup.TenantField == nil || *lookup.TenantField != "tenant_id" {
		t.Fatalf("expected tenant field")
	}
	if len(lookup.KeyFields) != 3 {
		t.Fatalf("expected metadata key fields")
	}
	if len(missing) != 2 || missing[0] != "auth_ref" {
		t.Fatalf("unexpected missing fields: %#v", missing)
	}

	resolution := ExternalSystemMetadataResolution{
		Record: VersionedRecord{
			Collection: CollectionName(lookup.MetadataEntity),
			Key:        "tenant-a/stripe/default",
			Version:    1,
			Document:   document,
		},
		MissingRequiredFields: missing,
	}
	if resolution.Complete() {
		t.Fatalf("expected incomplete metadata resolution")
	}
}
