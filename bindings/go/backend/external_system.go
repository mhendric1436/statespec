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

type ExternalSystemMetadataResolver interface {
	ResolveMetadata(ctx context.Context, backend Backend, lookup ExternalSystemMetadataLookup) (*ExternalSystemMetadataResolution, error)

	ResolveMetadataTx(ctx context.Context, tx Transaction, lookup ExternalSystemMetadataLookup) (*ExternalSystemMetadataResolution, error)
}
