package backend

import (
	"strconv"
	"strings"
)

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

	differences := []SchemaCompatibilityDifference{}
	add := func(reason SchemaCompatibilityReason, path string, message string) {
		differences = append(differences, SchemaCompatibilityDifference{
			Reason:  reason,
			Path:    path,
			Message: message,
		})
	}

	if existing.Name != requested.Name {
		add(CollectionNameChanged, "name", "collection name changed from '"+existing.Name+"' to '"+requested.Name+"'")
	}
	if requested.SchemaVersion < existing.SchemaVersion {
		add(SchemaVersionDecreased, "schema_version", "schema_version decreased")
	} else if requested.SchemaVersion == existing.SchemaVersion {
		add(SchemaVersionNotIncremented, "schema_version", "schema_version must increase when descriptor shape changes")
	}
	if !stringSlicesEqual(existing.KeyFields, requested.KeyFields) {
		add(KeyFieldsChanged, "key_fields", "key_fields changed")
	}

	for _, existingField := range existing.Fields {
		requestedField, ok := findFieldDescriptor(requested.Fields, existingField.Name)
		if !ok {
			add(FieldRemoved, "fields."+existingField.Name, "field '"+existingField.Name+"' was removed")
			continue
		}
		if existingField.Type != requestedField.Type {
			add(FieldTypeChanged, "fields."+existingField.Name, "field '"+existingField.Name+"' type changed")
		}
		if existingField.TypeName != requestedField.TypeName {
			add(FieldTypeNameChanged, "fields."+existingField.Name, "field '"+existingField.Name+"' type_name changed")
		}
		if !existingField.Required && requestedField.Required {
			add(FieldBecameRequired, "fields."+existingField.Name, "field '"+existingField.Name+"' became required")
		}
	}

	for _, requestedField := range requested.Fields {
		if _, ok := findFieldDescriptor(existing.Fields, requestedField.Name); !ok && requestedField.Required {
			add(RequiredFieldAdded, "fields."+requestedField.Name, "required field '"+requestedField.Name+"' was added")
		}
	}

	for _, existingIndex := range existing.Indexes {
		requestedIndex, ok := findIndexDescriptor(requested.Indexes, existingIndex.Name)
		if !ok {
			add(IndexRemoved, "indexes."+existingIndex.Name, "index '"+existingIndex.Name+"' was removed")
			continue
		}
		if !stringSlicesEqual(existingIndex.Fields, requestedIndex.Fields) {
			add(IndexFieldsChanged, "indexes."+existingIndex.Name, "index '"+existingIndex.Name+"' fields changed")
		}
		if existingIndex.Unique != requestedIndex.Unique {
			add(IndexUniquenessChanged, "indexes."+existingIndex.Name, "index '"+existingIndex.Name+"' uniqueness changed")
		}
	}

	for _, requestedIndex := range requested.Indexes {
		if _, ok := findIndexDescriptor(existing.Indexes, requestedIndex.Name); !ok && requestedIndex.Unique {
			add(UniqueIndexAdded, "indexes."+requestedIndex.Name, "unique index '"+requestedIndex.Name+"' was added")
		}
	}

	if len(differences) > 0 {
		return SchemaCompatibilityResult{
			Status:      SchemaCompatibilityIncompatible,
			Differences: differences,
		}
	}

	return SchemaCompatibilityResult{Status: SchemaCompatibilityCompatible}
}

func ValidateCollectionDescriptorUpgrade(existing CollectionDescriptor, requested CollectionDescriptor) error {
	result := CompareCollectionDescriptors(existing, requested)
	if result.Compatible() {
		return nil
	}
	message := "collection '" + requested.Name + "' schema upgrade from version " +
		strconv.FormatUint(existing.SchemaVersion, 10) + " to version " +
		strconv.FormatUint(requested.SchemaVersion, 10) + " is incompatible"
	if len(result.Differences) > 0 {
		parts := make([]string, 0, len(result.Differences))
		for _, difference := range result.Differences {
			parts = append(parts, string(difference.Reason)+" at "+difference.Path+": "+difference.Message)
		}
		message += ": " + strings.Join(parts, "; ")
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

func findFieldDescriptor(fields []FieldDescriptor, name string) (FieldDescriptor, bool) {
	for _, field := range fields {
		if field.Name == name {
			return field, true
		}
	}
	return FieldDescriptor{}, false
}

func findIndexDescriptor(indexes []IndexDescriptor, name string) (IndexDescriptor, bool) {
	for _, index := range indexes {
		if index.Name == name {
			return index, true
		}
	}
	return IndexDescriptor{}, false
}
