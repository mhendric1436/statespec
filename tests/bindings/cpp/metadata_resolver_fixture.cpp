#include "common/system_descriptors.hpp"

#include <string>
#include <vector>

class FakeTx final : public statespec::backend::ITransaction
{
  public:
    bool is_open() const override
    {
        return true;
    }
    void abort() override {}
};

class FakeResolver final : public statespec::backend::IExternalSystemMetadataResolver
{
  public:
    int calls = 0;

    std::optional<statespec::backend::ExternalSystemMetadataResolution> resolve_metadata(
        statespec::backend::IBackend&,
        const statespec::backend::ExternalSystemMetadataLookup&
    ) override
    {
        return std::nullopt;
    }

    std::optional<statespec::backend::ExternalSystemMetadataResolution> resolve_metadataTx(
        statespec::backend::ITransaction&,
        const statespec::backend::ExternalSystemMetadataLookup& lookup
    ) override
    {
        ++calls;
        const auto document = statespec::backend::Json::object(
            {{"tenant_id", "tenant-a"}, {"base_url", "https://api.stripe.test"}}
        );
        return statespec::backend::ExternalSystemMetadataResolution{
            statespec::backend::VersionedRecord{
                lookup.metadata_entity,
                "tenant-a/stripe/default",
                1,
                document,
            },
            statespec::backend::missing_required_metadata_fields(document, lookup.required_fields),
        };
    }
};

class FakeMappingApplicator final
    : public statespec_generated::IExternalSystemMetadataMappingApplicator
{
  public:
    statespec_generated::ExternalSystemMetadataMappingOutput apply(
        const statespec_generated::ExternalSystemMetadataMappingPlan& plan,
        const statespec_generated::ExternalSystemMetadataMappingInputs& inputs
    ) override
    {
        statespec_generated::ExternalSystemMetadataMappingOutput output;
        output.missing_sources =
            statespec_generated::missing_external_system_metadata_mapping_sources(plan, inputs);
        for (const auto& assignment : plan.all_mappings)
        {
            const auto* source = inputs.source_value(assignment);
            if (source == nullptr)
            {
                continue;
            }
            if (assignment.target_root == "client")
            {
                output.client_config.emplace(assignment.field, *source);
            }
            else if (assignment.target_root == "request")
            {
                output.request_payload.emplace(assignment.field, *source);
            }
        }
        return output;
    }
};

class FakeOperatorMetadataRepository final
    : public statespec_generated::IExternalSystemOperatorMetadataRepository
{
  public:
    std::optional<statespec::backend::VersionedRecord> upsert_metadataTx(
        statespec::backend::ITransaction&,
        const statespec_generated::ExternalSystemOperatorMetadataUpsertRequest& request
    ) override
    {
        return statespec::backend::VersionedRecord{
            request.lookup.metadata_entity,
            "tenant-a/stripe/default",
            request.expected_version.value_or(0) + 1,
            request.document,
        };
    }

    std::optional<statespec::backend::VersionedRecord> get_metadataTx(
        statespec::backend::ITransaction&,
        const statespec_generated::ExternalSystemOperatorMetadataGetRequest& request
    ) override
    {
        return statespec::backend::VersionedRecord{
            request.lookup.metadata_entity,
            "tenant-a/stripe/default",
            1,
            statespec::backend::Json::object({{"status", "Active"}}),
        };
    }

    std::optional<statespec::backend::VersionedRecord> disable_metadataTx(
        statespec::backend::ITransaction&,
        const statespec_generated::ExternalSystemOperatorMetadataDisableRequest& request
    ) override
    {
        return statespec::backend::VersionedRecord{
            request.lookup.metadata_entity,
            "tenant-a/stripe/default",
            request.expected_version.value_or(0) + 1,
            statespec::backend::Json::object({{"status", request.disabled_status}}),
        };
    }

    std::optional<statespec::backend::VersionedRecord> delete_metadataTx(
        statespec::backend::ITransaction&,
        const statespec_generated::ExternalSystemOperatorMetadataDeleteRequest& request
    ) override
    {
        return statespec::backend::VersionedRecord{
            request.lookup.metadata_entity,
            "tenant-a/stripe/default",
            request.expected_version.value_or(0) + 1,
            statespec::backend::Json::object({{"status", request.deleted_status}}),
        };
    }
};

class FakeOperatorMetadataApiHandler final
    : public statespec_generated::IExternalSystemOperatorMetadataApiHandler
{
  public:
    statespec_generated::ApiResponse handle_upsert_metadataTx(
        statespec::backend::ITransaction& tx,
        statespec_generated::IExternalSystemOperatorMetadataRepository& repository,
        const statespec_generated::ExternalSystemOperatorMetadataUpsertRequest& request
    ) override
    {
        auto record = repository.upsert_metadataTx(tx, request);
        return statespec_generated::ApiResponse{
            record.has_value() ? 200 : 404,
            statespec::backend::Json::object({{"operation", "upsert"}}),
        };
    }

    statespec_generated::ApiResponse handle_get_metadataTx(
        statespec::backend::ITransaction& tx,
        statespec_generated::IExternalSystemOperatorMetadataRepository& repository,
        const statespec_generated::ExternalSystemOperatorMetadataGetRequest& request
    ) override
    {
        auto record = repository.get_metadataTx(tx, request);
        return statespec_generated::ApiResponse{
            record.has_value() ? 200 : 404,
            statespec::backend::Json::object({{"operation", "get"}}),
        };
    }

    statespec_generated::ApiResponse handle_disable_metadataTx(
        statespec::backend::ITransaction& tx,
        statespec_generated::IExternalSystemOperatorMetadataRepository& repository,
        const statespec_generated::ExternalSystemOperatorMetadataDisableRequest& request
    ) override
    {
        auto record = repository.disable_metadataTx(tx, request);
        return statespec_generated::ApiResponse{
            record.has_value() ? 200 : 404,
            statespec::backend::Json::object({{"operation", "disable"}}),
        };
    }

    statespec_generated::ApiResponse handle_delete_metadataTx(
        statespec::backend::ITransaction& tx,
        statespec_generated::IExternalSystemOperatorMetadataRepository& repository,
        const statespec_generated::ExternalSystemOperatorMetadataDeleteRequest& request
    ) override
    {
        auto record = repository.delete_metadataTx(tx, request);
        return statespec_generated::ApiResponse{
            record.has_value() ? 200 : 404,
            statespec::backend::Json::object({{"operation", "delete"}}),
        };
    }
};

int main()
{
    FakeTx tx;
    FakeResolver resolver;
    const auto descriptors = statespec_generated::external_system_descriptors();
    const auto plan = statespec_generated::external_system_metadata_mapping_plan(descriptors[0]);
    if (plan.all_mappings.size() != 4 || plan.client_mappings.size() != 3 ||
        plan.request_mappings.size() != 1 || plan.client_mappings[0].source_root != "metadata" ||
        plan.client_mappings[0].source_field != "base_url" ||
        plan.client_mappings[0].target_root != "client" ||
        plan.client_mappings[0].field != "base_url" ||
        plan.request_mappings[0].source_root != "input" ||
        plan.request_mappings[0].source_field != "order_id" ||
        plan.request_mappings[0].target_root != "request" ||
        plan.request_mappings[0].field != "order_id")
    {
        return 1;
    }
    FakeMappingApplicator applicator;
    statespec_generated::IExternalSystemMetadataMappingApplicator& mapping_applicator = applicator;
    const auto mapped = mapping_applicator.apply(
        plan, statespec_generated::ExternalSystemMetadataMappingInputs{
                  {{"order_id", "order-1"}},
                  {},
                  {},
                  {{"base_url", "https://api.stripe.test"},
                   {"auth_ref", "secret:stripe"},
                   {"timeout_ms", std::int64_t{5000}}},
              }
    );
    if (mapped.client_config.size() != 3 || mapped.request_payload.size() != 1 ||
        !mapped.missing_sources.empty() ||
        mapped.client_config.find("base_url") == mapped.client_config.end() ||
        mapped.request_payload.find("order_id") == mapped.request_payload.end())
    {
        return 1;
    }
    const auto missing_mapped = mapping_applicator.apply(
        plan, statespec_generated::ExternalSystemMetadataMappingInputs{
                  {{"order_id", "order-1"}},
                  {},
                  {},
                  {{"base_url", "https://api.stripe.test"}, {"auth_ref", "secret:stripe"}},
              }
    );
    if (missing_mapped.missing_sources.size() != 1 ||
        missing_mapped.missing_sources[0].source != "metadata.timeout_ms" ||
        missing_mapped.missing_sources[0].target_root != "client" ||
        missing_mapped.missing_sources[0].field != "timeout_ms")
    {
        return 1;
    }
    std::vector<statespec::backend::ExternalSystemMetadataKeyValue> keys{
        {"tenant_id", "tenant-a"},
        {"external_system_id", "stripe"},
        {"profile", "default"},
    };
    auto lookup = statespec_generated::external_system_metadata_lookup("Stripe", keys);
    if (!lookup.has_value())
    {
        return 1;
    }
    FakeOperatorMetadataRepository repository;
    statespec_generated::IExternalSystemOperatorMetadataRepository& metadata_repository =
        repository;
    const auto upserted = metadata_repository.upsert_metadataTx(
        tx, statespec_generated::ExternalSystemOperatorMetadataUpsertRequest{
                *lookup,
                statespec::backend::Json::object({{"tenant_id", "tenant-a"}}),
                statespec::backend::Version{1},
            }
    );
    const auto loaded = metadata_repository.get_metadataTx(
        tx, statespec_generated::ExternalSystemOperatorMetadataGetRequest{*lookup}
    );
    const auto disabled = metadata_repository.disable_metadataTx(
        tx, statespec_generated::ExternalSystemOperatorMetadataDisableRequest{
                *lookup,
                statespec::backend::Version{2},
                "Disabled",
            }
    );
    const auto deleted = metadata_repository.delete_metadataTx(
        tx, statespec_generated::ExternalSystemOperatorMetadataDeleteRequest{
                *lookup,
                statespec::backend::Version{3},
                "Deleted",
            }
    );
    if (!upserted.has_value() || upserted->version != 2 || !loaded.has_value() ||
        !disabled.has_value() || disabled->version != 3 || !deleted.has_value() ||
        deleted->version != 4)
    {
        return 1;
    }
    FakeOperatorMetadataApiHandler operator_api_handler;
    statespec_generated::IExternalSystemOperatorMetadataApiHandler& metadata_api_handler =
        operator_api_handler;
    const auto api_response = metadata_api_handler.handle_upsert_metadataTx(
        tx, metadata_repository,
        statespec_generated::ExternalSystemOperatorMetadataUpsertRequest{
            *lookup,
            statespec::backend::Json::object({{"tenant_id", "tenant-a"}}),
            statespec::backend::Version{4},
        }
    );
    if (api_response.status_code != 200)
    {
        return 1;
    }
    auto resolved =
        statespec_generated::resolve_external_system_metadataTx(resolver, tx, "Stripe", keys);
    if (!resolved.has_value() || resolved->complete() || resolver.calls != 1)
    {
        return 1;
    }

    keys.pop_back();
    auto skipped =
        statespec_generated::resolve_external_system_metadataTx(resolver, tx, "Stripe", keys);
    return skipped.has_value() || resolver.calls != 1 ? 1 : 0;
}
