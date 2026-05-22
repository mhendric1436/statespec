package backend

import "testing"

func baseSchemaCompatibilityDescriptor() CollectionDescriptor {
	return CollectionDescriptor{
		Name: "orders",
		Fields: []FieldDescriptor{
			{Name: "tenant_id", Type: FieldTypeString, TypeName: "string", Required: true},
			{Name: "status", Type: FieldTypeString, TypeName: "string", Required: false},
		},
		KeyFields:     []string{"tenant_id"},
		Indexes:       []IndexDescriptor{{Name: "by_status", Fields: []string{"status"}}},
		SchemaVersion: 1,
	}
}

func requireSchemaReason(t *testing.T, requested CollectionDescriptor, reason SchemaCompatibilityReason) {
	t.Helper()
	result := CompareCollectionDescriptors(baseSchemaCompatibilityDescriptor(), requested)
	if result.Status != SchemaCompatibilityIncompatible {
		t.Fatalf("expected incompatible, got %s", result.Status)
	}
	for _, difference := range result.Differences {
		if difference.Reason == reason {
			return
		}
	}
	t.Fatalf("expected reason %s in %+v", reason, result.Differences)
}

func cloneSchemaDescriptor(descriptor CollectionDescriptor) CollectionDescriptor {
	clone := descriptor
	clone.Fields = append([]FieldDescriptor{}, descriptor.Fields...)
	clone.KeyFields = append([]string{}, descriptor.KeyFields...)
	clone.Indexes = append([]IndexDescriptor{}, descriptor.Indexes...)
	for index := range clone.Indexes {
		clone.Indexes[index].Fields = append([]string{}, descriptor.Indexes[index].Fields...)
	}
	return clone
}

func TestSchemaCompatibilityAllowsIdenticalAndCompatibleChanges(t *testing.T) {
	base := baseSchemaCompatibilityDescriptor()
	identical := CompareCollectionDescriptors(base, base)
	if identical.Status != SchemaCompatibilityIdentical || !identical.Compatible() {
		t.Fatalf("expected identical compatible result: %+v", identical)
	}

	withOptionalField := cloneSchemaDescriptor(base)
	withOptionalField.SchemaVersion = 2
	withOptionalField.Fields = append(withOptionalField.Fields, FieldDescriptor{
		Name: "description", Type: FieldTypeString, TypeName: "string",
	})
	if result := CompareCollectionDescriptors(base, withOptionalField); !result.Compatible() || result.Status != SchemaCompatibilityCompatible {
		t.Fatalf("expected optional field to be compatible: %+v", result)
	}

	withNonUniqueIndex := cloneSchemaDescriptor(base)
	withNonUniqueIndex.SchemaVersion = 2
	withNonUniqueIndex.Indexes = append(withNonUniqueIndex.Indexes, IndexDescriptor{
		Name: "by_tenant_status", Fields: []string{"tenant_id", "status"},
	})
	if result := CompareCollectionDescriptors(base, withNonUniqueIndex); !result.Compatible() {
		t.Fatalf("expected non-unique index to be compatible: %+v", result)
	}
}

func TestSchemaCompatibilityRejectsIncompatibleChanges(t *testing.T) {
	base := baseSchemaCompatibilityDescriptor()

	sameVersion := cloneSchemaDescriptor(base)
	sameVersion.Fields = append(sameVersion.Fields, FieldDescriptor{Name: "description", Type: FieldTypeString, TypeName: "string"})
	requireSchemaReason(t, sameVersion, SchemaVersionNotIncremented)

	decreasedVersion := cloneSchemaDescriptor(sameVersion)
	decreasedVersion.SchemaVersion = 0
	requireSchemaReason(t, decreasedVersion, SchemaVersionDecreased)

	keyChanged := cloneSchemaDescriptor(base)
	keyChanged.SchemaVersion = 2
	keyChanged.KeyFields = []string{"tenant_id", "order_id"}
	requireSchemaReason(t, keyChanged, KeyFieldsChanged)

	fieldRemoved := cloneSchemaDescriptor(base)
	fieldRemoved.SchemaVersion = 2
	fieldRemoved.Fields = fieldRemoved.Fields[:1]
	requireSchemaReason(t, fieldRemoved, FieldRemoved)

	fieldTypeChanged := cloneSchemaDescriptor(base)
	fieldTypeChanged.SchemaVersion = 2
	fieldTypeChanged.Fields[1].Type = FieldTypeInt
	requireSchemaReason(t, fieldTypeChanged, FieldTypeChanged)

	fieldTypeNameChanged := cloneSchemaDescriptor(base)
	fieldTypeNameChanged.SchemaVersion = 2
	fieldTypeNameChanged.Fields[1].TypeName = "Status"
	requireSchemaReason(t, fieldTypeNameChanged, FieldTypeNameChanged)

	fieldBecameRequired := cloneSchemaDescriptor(base)
	fieldBecameRequired.SchemaVersion = 2
	fieldBecameRequired.Fields[1].Required = true
	requireSchemaReason(t, fieldBecameRequired, FieldBecameRequired)

	requiredFieldAdded := cloneSchemaDescriptor(base)
	requiredFieldAdded.SchemaVersion = 2
	requiredFieldAdded.Fields = append(requiredFieldAdded.Fields, FieldDescriptor{Name: "priority", Type: FieldTypeInt, TypeName: "int", Required: true})
	requireSchemaReason(t, requiredFieldAdded, RequiredFieldAdded)

	indexRemoved := cloneSchemaDescriptor(base)
	indexRemoved.SchemaVersion = 2
	indexRemoved.Indexes = nil
	requireSchemaReason(t, indexRemoved, IndexRemoved)

	indexFieldsChanged := cloneSchemaDescriptor(base)
	indexFieldsChanged.SchemaVersion = 2
	indexFieldsChanged.Indexes[0].Fields = []string{"tenant_id", "status"}
	requireSchemaReason(t, indexFieldsChanged, IndexFieldsChanged)

	indexUniquenessChanged := cloneSchemaDescriptor(base)
	indexUniquenessChanged.SchemaVersion = 2
	indexUniquenessChanged.Indexes[0].Unique = true
	requireSchemaReason(t, indexUniquenessChanged, IndexUniquenessChanged)

	uniqueIndexAdded := cloneSchemaDescriptor(base)
	uniqueIndexAdded.SchemaVersion = 2
	uniqueIndexAdded.Indexes = append(uniqueIndexAdded.Indexes, IndexDescriptor{Name: "unique_status", Fields: []string{"status"}, Unique: true})
	requireSchemaReason(t, uniqueIndexAdded, UniqueIndexAdded)
}
