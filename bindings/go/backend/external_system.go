package backend

import "context"

type ExternalSystemMetadataKeyValue struct {
	Field string
	Value JSON
}

type ExternalSystemMetadataLookup struct {
	ExternalSystem string
	MetadataEntity string
	TenantField    *string
	ProfileField   string
	KeyFields      []string
	KeyValues      []ExternalSystemMetadataKeyValue
	RequiredFields []string
}

func (l ExternalSystemMetadataLookup) KeyComplete() bool {
	return len(MissingMetadataKeyFields(l)) == 0
}

type ExternalSystemMetadataResolution struct {
	Record                VersionedRecord
	MissingRequiredFields []string
}

func (r ExternalSystemMetadataResolution) Complete() bool {
	return len(r.MissingRequiredFields) == 0
}

func MissingRequiredMetadataFields(document JSON, requiredFields []string) []string {
	missing := []string{}
	for _, field := range requiredFields {
		if _, ok := document.Find(field); !ok {
			missing = append(missing, field)
		}
	}
	return missing
}

func MissingMetadataKeyFields(lookup ExternalSystemMetadataLookup) []string {
	provided := map[string]bool{}
	for _, keyValue := range lookup.KeyValues {
		provided[keyValue.Field] = true
	}

	missing := []string{}
	for _, keyField := range lookup.KeyFields {
		if !provided[keyField] {
			missing = append(missing, keyField)
		}
	}
	return missing
}

type ExternalSystemMetadataResolver interface {
	ResolveMetadata(ctx context.Context, backend Backend, lookup ExternalSystemMetadataLookup) (*ExternalSystemMetadataResolution, error)

	ResolveMetadataTx(ctx context.Context, tx Transaction, lookup ExternalSystemMetadataLookup) (*ExternalSystemMetadataResolution, error)
}
