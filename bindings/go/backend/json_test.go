package backend

import "testing"

func TestParseJSONObjectsAndArrays(t *testing.T) {
	value, err := ParseJSON(`{"active":true,"count":7,"tags":["a",null,2.5]}`)
	if err != nil {
		t.Fatalf("ParseJSON failed: %v", err)
	}

	object, ok := value.AsObject()
	if !ok {
		t.Fatalf("expected object")
	}

	active, _ := object["active"].AsBool()
	count, _ := object["count"].AsInt()
	array, _ := object["tags"].AsArray()
	decimal, _ := array[2].AsFloat()

	if !active {
		t.Fatalf("expected active=true")
	}
	if count != 7 {
		t.Fatalf("expected count=7 got %d", count)
	}
	if len(array) != 3 {
		t.Fatalf("expected 3 array values")
	}
	if decimal != 2.5 {
		t.Fatalf("expected decimal=2.5 got %f", decimal)
	}
}

func TestCanonicalStringIsStable(t *testing.T) {
	value := JSONObject(map[string]JSON{
		"z": JSONInt(1),
		"a": JSONArray([]JSON{JSONBool(true), JSONString("x")}),
	})

	canonical := value.CanonicalString()
	if canonical != `{"a":[true,"x"],"z":1}` {
		t.Fatalf("unexpected canonical string %q", canonical)
	}

	reparsed, err := ParseJSON(canonical)
	if err != nil {
		t.Fatalf("ParseJSON failed: %v", err)
	}

	if reparsed.CanonicalString() != canonical {
		t.Fatalf("canonical serialization mismatch")
	}
}

func TestParseJSONRejectsDuplicateKeys(t *testing.T) {
	_, err := ParseJSON(`{"a":1,"a":2}`)
	if err == nil {
		t.Fatalf("expected duplicate key error")
	}
}

func TestParseJSONRejectsTrailingContent(t *testing.T) {
	_, err := ParseJSON(`{} []`)
	if err == nil {
		t.Fatalf("expected trailing content error")
	}
}

func TestBackendTypedJSONSurfaces(t *testing.T) {
	query := JSONEqualsQuery("$.active", JSONBool(true))
	if query.JSONValue == nil {
		t.Fatalf("expected JSONValue")
	}

	value, ok := query.JSONValue.AsBool()
	if !ok || !value {
		t.Fatalf("expected bool JSON query value")
	}

	indexValue := StringIndexValue("alice@example.com")
	stringValue, ok := indexValue.Value.AsString()
	if !ok || stringValue != "alice@example.com" {
		t.Fatalf("unexpected index value")
	}
}
