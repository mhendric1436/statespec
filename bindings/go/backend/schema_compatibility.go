package backend

type SchemaCompatibilityStatus string

const (
	SchemaCompatibilityIdentical    SchemaCompatibilityStatus = "Identical"
	SchemaCompatibilityCompatible   SchemaCompatibilityStatus = "Compatible"
	SchemaCompatibilityIncompatible SchemaCompatibilityStatus = "Incompatible"
)

type SchemaCompatibilityReason string

const (
	SchemaVersionDecreased      SchemaCompatibilityReason = "SchemaVersionDecreased"
	SchemaVersionNotIncremented SchemaCompatibilityReason = "SchemaVersionNotIncremented"
	CollectionNameChanged       SchemaCompatibilityReason = "CollectionNameChanged"
	KeyFieldsChanged            SchemaCompatibilityReason = "KeyFieldsChanged"
	FieldRemoved                SchemaCompatibilityReason = "FieldRemoved"
	FieldRenamed                SchemaCompatibilityReason = "FieldRenamed"
	FieldTypeChanged            SchemaCompatibilityReason = "FieldTypeChanged"
	FieldTypeNameChanged        SchemaCompatibilityReason = "FieldTypeNameChanged"
	RequiredFieldAdded          SchemaCompatibilityReason = "RequiredFieldAdded"
	FieldBecameRequired         SchemaCompatibilityReason = "FieldBecameRequired"
	IndexRemoved                SchemaCompatibilityReason = "IndexRemoved"
	IndexRenamed                SchemaCompatibilityReason = "IndexRenamed"
	IndexFieldsChanged          SchemaCompatibilityReason = "IndexFieldsChanged"
	IndexUniquenessChanged      SchemaCompatibilityReason = "IndexUniquenessChanged"
	UniqueIndexAdded            SchemaCompatibilityReason = "UniqueIndexAdded"
)

type SchemaCompatibilityDifference struct {
	Reason  SchemaCompatibilityReason
	Path    string
	Message string
}

type SchemaCompatibilityResult struct {
	Status      SchemaCompatibilityStatus
	Differences []SchemaCompatibilityDifference
}

func (r SchemaCompatibilityResult) Compatible() bool {
	return r.Status != SchemaCompatibilityIncompatible
}

func CompareCollectionDescriptors(existing CollectionDescriptor, requested CollectionDescriptor) SchemaCompatibilityResult {
	if collectionDescriptorsEqual(existing, requested) {
		return SchemaCompatibilityResult{Status: SchemaCompatibilityIdentical}
	}
	return SchemaCompatibilityResult{Status: SchemaCompatibilityCompatible}
}

func ValidateCollectionDescriptorUpgrade(existing CollectionDescriptor, requested CollectionDescriptor) error {
	result := CompareCollectionDescriptors(existing, requested)
	if result.Compatible() {
		return nil
	}
	message := "collection schema upgrade is incompatible"
	if len(result.Differences) > 0 {
		message += ": " + result.Differences[0].Message
	}
	return ConflictError{Kind: SchemaConflict, Message: message}
}

func collectionDescriptorsEqual(left CollectionDescriptor, right CollectionDescriptor) bool {
	if left.Name != right.Name || left.SchemaVersion != right.SchemaVersion ||
		!stringSlicesEqual(left.KeyFields, right.KeyFields) ||
		len(left.Fields) != len(right.Fields) || len(left.Indexes) != len(right.Indexes) {
		return false
	}
	for index := range left.Fields {
		if left.Fields[index] != right.Fields[index] {
			return false
		}
	}
	for index := range left.Indexes {
		if left.Indexes[index].Name != right.Indexes[index].Name ||
			left.Indexes[index].Unique != right.Indexes[index].Unique ||
			!stringSlicesEqual(left.Indexes[index].Fields, right.Indexes[index].Fields) {
			return false
		}
	}
	return true
}

func stringSlicesEqual(left []string, right []string) bool {
	if len(left) != len(right) {
		return false
	}
	for index := range left {
		if left[index] != right[index] {
			return false
		}
	}
	return true
}
