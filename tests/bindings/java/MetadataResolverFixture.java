package com.statespec.generated;

import com.statespec.backend.Backend;
import com.statespec.backend.ExternalSystem;
import com.statespec.backend.Json;
import com.statespec.generated.external.metadata.*;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public final class MetadataResolverFixture
{
    private static final class FixtureTx implements Backend.Transaction
    {
        @Override public boolean isOpen()
        {
            return true;
        }
        @Override public void abort() throws Backend.BackendException {}

        @Override
        public Optional<Backend.VersionedRecord>
        get(String collection,
            String key) throws Backend.BackendException
        {
            throw new Backend.BackendException("unsupported fixture transaction get");
        }

        @Override
        public List<Backend.VersionedRecord> query(
            String collection,
            Backend.Query query
        ) throws Backend.BackendException
        {
            throw new Backend.BackendException("unsupported fixture transaction query");
        }

        @Override
        public void
        put(String collection,
            String key,
            Json document) throws Backend.BackendException
        {
            throw new Backend.BackendException("unsupported fixture transaction put");
        }

        @Override
        public void erase(
            String collection,
            String key
        ) throws Backend.BackendException
        {
            throw new Backend.BackendException("unsupported fixture transaction erase");
        }
    }

    private static final class FixtureResolver implements ExternalSystem
    {
        int calls = 0;

        @Override
        public Optional<MetadataResolution> resolveMetadata(
            Backend backend,
            MetadataLookup lookup
        ) throws Backend.BackendException
        {
            return Optional.empty();
        }

        @Override
        public Optional<MetadataResolution> resolveMetadataTx(
            Backend.Transaction tx,
            MetadataLookup lookup
        ) throws Backend.BackendException
        {
            calls++;
            Json document = Json.object(Map.of(
                "tenant_id", Json.string("tenant-a"), "base_url",
                Json.string("https://api.stripe.test")
            ));
            return Optional.of(new MetadataResolution(
                new Backend.VersionedRecord(
                    lookup.metadataEntity(), "tenant-a/stripe/default", 1L, document
                ),
                ExternalSystem.missingRequiredMetadataFields(document, lookup.requiredFields())
            ));
        }
    }

    private static final class FixtureMappingApplicator
        implements ExternalSystemMetadataMappingApplicator
    {
        @Override
        public ExternalSystemMetadataMappingOutput applyExternalSystemMetadataMappings(
            ExternalSystemMetadataMappingPlan plan,
            ExternalSystemMetadataMappingInputs inputs
        )
        {
            java.util.HashMap<String, Json> clientConfig = new java.util.HashMap<>();
            java.util.HashMap<String, Json> requestPayload = new java.util.HashMap<>();
            for (ExternalSystemMetadataMappingAssignment assignment :
                 plan.allMappings())
            {
                Optional<Json> value = inputs.assignmentValue(assignment);
                if (value.isEmpty())
                {
                    continue;
                }
                if (assignment.targetRoot().equals("client"))
                {
                    clientConfig.put(assignment.field(), value.orElseThrow());
                }
                else if (assignment.targetRoot().equals("request"))
                {
                    requestPayload.put(assignment.field(), value.orElseThrow());
                }
            }
            return new ExternalSystemMetadataMappingOutput(
                clientConfig, requestPayload,
                ExternalSystemMetadata.missingExternalSystemMetadataMappingSources(plan, inputs)
            );
        }
    }

    private static final class FixtureOperatorMetadataRepository
        implements ExternalSystemOperatorMetadataRepository
    {
        @Override
        public Optional<Backend.VersionedRecord> upsertMetadataTx(
            Backend.Transaction tx,
            ExternalSystemOperatorMetadataUpsertRequest request
        )
        {
            return Optional.of(new Backend.VersionedRecord(
                request.lookup().metadataEntity(), "tenant-a/stripe/default",
                request.expectedVersion().orElse(0L) + 1L, request.document()
            ));
        }

        @Override
        public Optional<Backend.VersionedRecord> getMetadataTx(
            Backend.Transaction tx,
            ExternalSystemOperatorMetadataGetRequest request
        )
        {
            return Optional.of(new Backend.VersionedRecord(
                request.lookup().metadataEntity(), "tenant-a/stripe/default", 1L,
                Json.object(Map.of("status", Json.string("Active")))
            ));
        }

        @Override
        public Optional<Backend.VersionedRecord> disableMetadataTx(
            Backend.Transaction tx,
            ExternalSystemOperatorMetadataDisableRequest request
        )
        {
            return Optional.of(new Backend.VersionedRecord(
                request.lookup().metadataEntity(), "tenant-a/stripe/default",
                request.expectedVersion().orElse(0L) + 1L,
                Json.object(Map.of("status", Json.string(request.disabledStatus())))
            ));
        }

        @Override
        public Optional<Backend.VersionedRecord> deleteMetadataTx(
            Backend.Transaction tx,
            ExternalSystemOperatorMetadataDeleteRequest request
        )
        {
            return Optional.of(new Backend.VersionedRecord(
                request.lookup().metadataEntity(), "tenant-a/stripe/default",
                request.expectedVersion().orElse(0L) + 1L,
                Json.object(Map.of("status", Json.string(request.deletedStatus())))
            ));
        }
    }

    private static final class FixtureOperatorMetadataApiHandler
        implements ExternalSystemOperatorMetadataApiHandler
    {
        @Override
        public Descriptors.ApiResponse handleUpsertMetadataTx(
            Backend.Transaction tx,
            ExternalSystemOperatorMetadataRepository repository,
            ExternalSystemOperatorMetadataUpsertRequest request
        ) throws Exception
        {
            Optional<Backend.VersionedRecord> record = repository.upsertMetadataTx(tx, request);
            return new Descriptors.ApiResponse(
                record.isPresent() ? 200 : 404,
                Json.object(Map.of("operation", Json.string("upsert")))
            );
        }

        @Override
        public Descriptors.ApiResponse handleGetMetadataTx(
            Backend.Transaction tx,
            ExternalSystemOperatorMetadataRepository repository,
            ExternalSystemOperatorMetadataGetRequest request
        ) throws Exception
        {
            Optional<Backend.VersionedRecord> record = repository.getMetadataTx(tx, request);
            return new Descriptors.ApiResponse(
                record.isPresent() ? 200 : 404, Json.object(Map.of("operation", Json.string("get")))
            );
        }

        @Override
        public Descriptors.ApiResponse handleDisableMetadataTx(
            Backend.Transaction tx,
            ExternalSystemOperatorMetadataRepository repository,
            ExternalSystemOperatorMetadataDisableRequest request
        ) throws Exception
        {
            Optional<Backend.VersionedRecord> record = repository.disableMetadataTx(tx, request);
            return new Descriptors.ApiResponse(
                record.isPresent() ? 200 : 404,
                Json.object(Map.of("operation", Json.string("disable")))
            );
        }

        @Override
        public Descriptors.ApiResponse handleDeleteMetadataTx(
            Backend.Transaction tx,
            ExternalSystemOperatorMetadataRepository repository,
            ExternalSystemOperatorMetadataDeleteRequest request
        ) throws Exception
        {
            Optional<Backend.VersionedRecord> record = repository.deleteMetadataTx(tx, request);
            return new Descriptors.ApiResponse(
                record.isPresent() ? 200 : 404,
                Json.object(Map.of("operation", Json.string("delete")))
            );
        }
    }

    public static void main(String[] args) throws Exception
    {
        FixtureResolver resolver = new FixtureResolver();
        FixtureTx tx = new FixtureTx();
        ExternalSystemMetadataMappingPlan plan =
            ExternalSystemMetadata.externalSystemMetadataMappingPlan(
                Descriptors.externalSystemDescriptors().get(0)
            );
        if (plan.allMappings().size() != 4 || plan.clientMappings().size() != 3 ||
            plan.requestMappings().size() != 1 ||
            !plan.clientMappings().get(0).sourceRoot().equals("metadata") ||
            !plan.clientMappings().get(0).sourceField().equals("base_url") ||
            !plan.clientMappings().get(0).targetRoot().equals("client") ||
            !plan.clientMappings().get(0).field().equals("base_url") ||
            !plan.requestMappings().get(0).sourceRoot().equals("input") ||
            !plan.requestMappings().get(0).sourceField().equals("order_id") ||
            !plan.requestMappings().get(0).targetRoot().equals("request") ||
            !plan.requestMappings().get(0).field().equals("order_id"))
        {
            throw new AssertionError("unexpected metadata mapping plan");
        }
        ExternalSystemMetadataMappingApplicator applicator =
            new FixtureMappingApplicator();
        ExternalSystemMetadataMappingOutput mapped =
            applicator.applyExternalSystemMetadataMappings(
                plan, new ExternalSystemMetadataMappingInputs(
                          Map.of("order_id", Json.string("order-1")), Map.of(), Map.of(),
                          Map.of(
                              "base_url", Json.string("https://api.stripe.test"), "auth_ref",
                              Json.string("secret:stripe"), "timeout_ms", Json.integer(5000)
                          )
                      )
            );
        if (mapped.clientConfig().size() != 3 || mapped.requestPayload().size() != 1 ||
            !mapped.missingSources().isEmpty())
        {
            throw new AssertionError("unexpected mapped metadata output");
        }
        ExternalSystemMetadataMappingOutput missingMapped =
            applicator.applyExternalSystemMetadataMappings(
                plan, new ExternalSystemMetadataMappingInputs(
                          Map.of("order_id", Json.string("order-1")), Map.of(), Map.of(),
                          Map.of(
                              "base_url", Json.string("https://api.stripe.test"), "auth_ref",
                              Json.string("secret:stripe")
                          )
                      )
            );
        if (missingMapped.missingSources().size() != 1 ||
            !missingMapped.missingSources().get(0).source().equals("metadata.timeout_ms") ||
            !missingMapped.missingSources().get(0).targetRoot().equals("client") ||
            !missingMapped.missingSources().get(0).field().equals("timeout_ms"))
        {
            throw new AssertionError("unexpected missing mapping diagnostics");
        }
        List<ExternalSystem.MetadataKeyValue> keys = List.of(
            new ExternalSystem.MetadataKeyValue("tenant_id", Json.string("tenant-a")),
            new ExternalSystem.MetadataKeyValue("external_system_id", Json.string("stripe")),
            new ExternalSystem.MetadataKeyValue("profile", Json.string("default"))
        );
        ExternalSystem.MetadataLookup lookup =
            ExternalSystemMetadata.externalSystemMetadataLookup("Stripe", keys).orElseThrow();
        ExternalSystemOperatorMetadataRepository repository =
            new FixtureOperatorMetadataRepository();
        Backend.VersionedRecord upserted =
            repository
                .upsertMetadataTx(
                    tx, new ExternalSystemOperatorMetadataUpsertRequest(
                            lookup, Json.object(Map.of("tenant_id", Json.string("tenant-a"))),
                            Optional.of(1L)
                        )
                )
                .orElseThrow();
        Backend.VersionedRecord loaded =
            repository
                .getMetadataTx(tx, new ExternalSystemOperatorMetadataGetRequest(lookup))
                .orElseThrow();
        Backend.VersionedRecord disabled =
            repository
                .disableMetadataTx(
                    tx, new ExternalSystemOperatorMetadataDisableRequest(
                            lookup, Optional.of(upserted.version()), "Disabled"
                        )
                )
                .orElseThrow();
        Backend.VersionedRecord deleted =
            repository
                .deleteMetadataTx(
                    tx, new ExternalSystemOperatorMetadataDeleteRequest(
                            lookup, Optional.of(disabled.version()), "Deleted"
                        )
                )
                .orElseThrow();
        if (upserted.version() != 2L || loaded.version() != 1L || disabled.version() != 3L ||
            deleted.version() != 4L)
        {
            throw new AssertionError("unexpected metadata repository result");
        }
        ExternalSystemOperatorMetadataApiHandler metadataApiHandler =
            new FixtureOperatorMetadataApiHandler();
        Descriptors.ApiResponse metadataApiResponse = metadataApiHandler.handleUpsertMetadataTx(
            tx, repository,
            new ExternalSystemOperatorMetadataUpsertRequest(
                lookup, Json.object(Map.of("tenant_id", Json.string("tenant-a"))),
                Optional.of(deleted.version())
            )
        );
        if (metadataApiResponse.statusCode() != 200)
        {
            throw new AssertionError("unexpected operator metadata API response");
        }
        Optional<ExternalSystem.MetadataResolution> resolved =
            ExternalSystemMetadata.resolveExternalSystemMetadataTx(resolver, tx, "Stripe", keys);
        if (resolved.isEmpty() || resolved.orElseThrow().complete() || resolver.calls != 1)
        {
            throw new AssertionError("expected incomplete metadata resolution through resolver");
        }

        Optional<ExternalSystem.MetadataResolution> skipped =
            ExternalSystemMetadata.resolveExternalSystemMetadataTx(resolver, tx, "Stripe", keys.subList(0, 2));
        if (skipped.isPresent() || resolver.calls != 1)
        {
            throw new AssertionError("expected incomplete key to skip resolver");
        }
    }
}
