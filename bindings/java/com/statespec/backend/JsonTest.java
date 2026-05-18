package com.statespec.backend;

import java.math.BigDecimal;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public final class JsonTest
{
    public static void main(String[] args)
    {
        parsesObjectsAndArrays();
        decodesEscapesAndUnicode();
        canonicalStringIsStable();
        rejectsMalformedInput();
        backendSurfacesUseTypedJson();
        featureFlagBindingsExposeTypedValues();
        externalSystemMetadataBindingsExposeLookupContracts();
        logBindingsExposeTypedEvents();
        metricBindingsExposeTypedSamples();
    }

    private static void parsesObjectsAndArrays()
    {
        Json parsed = Json.parse("{\"active\":true,\"count\":7,\"tags\":[\"a\",null,2.5]}");

        Json.ObjectValue object = requireType(parsed, Json.ObjectValue.class);
        Json.BooleanValue active =
            requireType(object.values().get("active"), Json.BooleanValue.class);
        Json.IntegerValue count =
            requireType(object.values().get("count"), Json.IntegerValue.class);
        Json.ArrayValue tags = requireType(object.values().get("tags"), Json.ArrayValue.class);
        Json.DecimalValue decimal = requireType(tags.values().get(2), Json.DecimalValue.class);

        require(active.value(), "active should be true");
        require(count.value() == 7, "count should be 7");
        require(tags.values().size() == 3, "tags should have three values");
        require(decimal.value().compareTo(new BigDecimal("2.5")) == 0, "decimal should be 2.5");
    }

    private static void decodesEscapesAndUnicode()
    {
        Json parsed = Json.parse(
            "{\"text\":\"line\\nA\\\\\\\"\",\"latin\":\"\\u00e9\",\"emoji\":\"\\ud83d\\ude00\"}"
        );
        Json.ObjectValue object = requireType(parsed, Json.ObjectValue.class);

        require(
            requireType(object.values().get("text"), Json.StringValue.class)
                .value()
                .equals("line\nA\\\""),
            "text escapes should decode"
        );
        require(
            requireType(object.values().get("latin"), Json.StringValue.class).value().equals("é"),
            "latin unicode should decode"
        );
        require(
            requireType(object.values().get("emoji"), Json.StringValue.class).value().equals("😀"),
            "surrogate pair should decode"
        );
    }

    private static void canonicalStringIsStable()
    {
        Json value = Json.object(Map.of(
            "z", Json.integer(1), "a", Json.array(List.of(Json.bool(true), Json.string("x")))
        ));

        String canonical = value.canonicalString();
        require(
            canonical.equals("{\"a\":[true,\"x\"],\"z\":1}"), "canonical string should sort keys"
        );
        require(Json.parse(canonical).equals(value), "canonical string should round trip");
    }

    private static void rejectsMalformedInput()
    {
        requireThrows(() -> Json.parse("[1,]"), "trailing comma should fail");
        requireThrows(() -> Json.parse("{\"a\":1,\"a\":2}"), "duplicate key should fail");
        requireThrows(() -> Json.parse("01"), "leading zero should fail");
        requireThrows(() -> Json.parse("-01"), "negative leading zero should fail");
        requireThrows(() -> Json.parse("\"raw\nnewline\""), "raw newline should fail");
        requireThrows(() -> Json.parse("\"\\uD800\""), "missing low surrogate should fail");
        requireThrows(() -> Json.parse("\"\\uDC00\""), "unexpected low surrogate should fail");
    }

    private static void backendSurfacesUseTypedJson()
    {
        Json document = Json.object(Map.of("id", Json.string("user:1"), "active", Json.bool(true)));
        Backend.VersionedRecord record =
            new Backend.VersionedRecord("users", "user:1", 3L, document);
        require(
            record.document().find("id").isPresent(),
            "versioned record document should be typed JSON"
        );

        Backend.Query.JsonEquals query = new Backend.Query.JsonEquals("$.active", Json.bool(true));
        require(
            query.value() instanceof Json.BooleanValue, "JSON equality value should be typed JSON"
        );

        Backend.IndexValue.StringValue indexValue =
            new Backend.IndexValue.StringValue("alice@example.com");
        require(
            indexValue.value() instanceof Json.StringValue, "index value should expose typed JSON"
        );
    }

    private static void featureFlagBindingsExposeTypedValues()
    {
        FeatureFlag.Value.BoolValue boolValue = new FeatureFlag.Value.BoolValue(true);
        FeatureFlag.Value.StringValue stringValue = new FeatureFlag.Value.StringValue("new");
        FeatureFlag.Value.IntValue intValue = new FeatureFlag.Value.IntValue(100L);
        FeatureFlag.Value.DecimalValue decimalValue = new FeatureFlag.Value.DecimalValue("1.5");

        require(boolValue.type() == FeatureFlag.Type.BOOL, "bool value should expose bool type");
        require(boolValue.asBool().orElse(false), "bool accessor should return value");
        require(boolValue.asString().isEmpty(), "bool value should not expose string accessor");
        require(
            stringValue.asString().orElseThrow().equals("new"),
            "string accessor should return value"
        );
        require(intValue.asInt().orElseThrow() == 100L, "int accessor should return value");
        require(
            decimalValue.asDecimal().orElseThrow().compareTo(new BigDecimal("1.5")) == 0,
            "decimal accessor should return value"
        );

        FeatureFlag.Definition definition = new FeatureFlag.Definition(
            "NewScheduler", FeatureFlag.Type.BOOL, new FeatureFlag.Value.BoolValue(false),
            FeatureFlag.ScopeKind.TENANT, Optional.of("platform"),
            Optional.of("Route through the new scheduler"), Optional.of("2026-12-31")
        );
        FeatureFlag.EvaluationContext context = new FeatureFlag.EvaluationContext(
            Optional.of("tenant-a"), Optional.empty(), Optional.empty(), Optional.empty()
        );

        require(definition.name().equals("NewScheduler"), "definition should expose name");
        require(
            definition.defaultValue().asBool().orElseThrow().equals(false),
            "definition should expose default"
        );

        FeatureFlag.RegisterDefinitionResult registration =
            new FeatureFlag.RegisterDefinitionResult(true, definition);
        require(registration.registeredNew(), "registration should expose created state");
        require(
            registration.definition().name().equals("NewScheduler"),
            "registration should expose definition"
        );

        FeatureFlag.EvaluationRequest request =
            new FeatureFlag.EvaluationRequest("NewScheduler", context);
        require(
            context.tenantId().orElseThrow().equals("tenant-a"), "context should expose tenant"
        );
        require(request.name().equals("NewScheduler"), "request should expose flag name");
        require(
            request.context().tenantId().orElseThrow().equals("tenant-a"),
            "request should expose context"
        );
    }

    private static void externalSystemMetadataBindingsExposeLookupContracts()
    {
        ExternalSystem.MetadataLookup lookup = new ExternalSystem.MetadataLookup(
            "Billing.Stripe", "ExternalSystemEndpoint", Optional.of("tenant_id"), "profile",
            List.of("tenant_id", "external_system_id", "profile"),
            List.of(
                new ExternalSystem.MetadataKeyValue("tenant_id", Json.string("tenant-a")),
                new ExternalSystem.MetadataKeyValue("external_system_id", Json.string("stripe")),
                new ExternalSystem.MetadataKeyValue("profile", Json.string("default"))
            ),
            List.of("base_url", "auth_ref", "timeout_ms")
        );

        Json document = Json.object(Map.of(
            "tenant_id", Json.string("tenant-a"), "base_url", Json.string("https://api.stripe.test")
        ));
        List<String> missing =
            ExternalSystem.missingRequiredMetadataFields(document, lookup.requiredFields());

        require(
            lookup.tenantField().orElseThrow().equals("tenant_id"),
            "lookup should expose tenant field"
        );
        require(lookup.keyFields().size() == 3, "lookup should expose key fields");
        require(
            missing.size() == 2 && missing.get(0).equals("auth_ref"),
            "required metadata inspection should report missing fields"
        );
        require(lookup.keyComplete(), "lookup key should be complete");

        ExternalSystem.MetadataLookup incompleteLookup = new ExternalSystem.MetadataLookup(
            lookup.externalSystem(), lookup.metadataEntity(), lookup.tenantField(),
            lookup.profileField(), lookup.keyFields(), lookup.keyValues().subList(0, 2),
            lookup.requiredFields()
        );
        List<String> missingKeyFields = ExternalSystem.missingMetadataKeyFields(incompleteLookup);
        require(!incompleteLookup.keyComplete(), "lookup key should report missing fields");
        require(
            missingKeyFields.size() == 1 && missingKeyFields.get(0).equals("profile"),
            "lookup key inspection should report missing profile"
        );

        ExternalSystem.MetadataResolution resolution = new ExternalSystem.MetadataResolution(
            new Backend.VersionedRecord(
                lookup.metadataEntity(), "tenant-a/stripe/default", 1L, document
            ),
            missing
        );
        require(!resolution.complete(), "resolution should expose incomplete metadata");
    }

    private static void logBindingsExposeTypedEvents()
    {
        Log.Event event = new Log.Event(
            "WorkflowLaunchDecision", Log.Level.INFO, "workflow.launch.decision",
            Map.of("tenant_id", Json.string("tenant-a"), "decision", Json.string("accepted"))
        );

        require(event.level() == Log.Level.INFO, "event should expose level");
        require(
            event.eventName().equals("workflow.launch.decision"),
            "event should expose backend event name"
        );
        require(
            event.fields().get("tenant_id").equals(Json.string("tenant-a")),
            "event fields should use typed JSON"
        );
    }

    private static void metricBindingsExposeTypedSamples()
    {
        Metric.Sample sample = new Metric.Sample(
            "WorkflowLaunchAttempts", Metric.Kind.COUNTER, "workflow_launch_attempts_total", 1.0,
            "count", Map.of("workflow_name", Json.string("OrderProcessing"))
        );

        require(sample.kind() == Metric.Kind.COUNTER, "sample should expose kind");
        require(
            sample.backendName().equals("workflow_launch_attempts_total"),
            "sample should expose backend name"
        );
        require(sample.value() == 1.0, "sample should expose value");
        require(
            sample.labels().get("workflow_name").equals(Json.string("OrderProcessing")),
            "sample labels should use typed JSON"
        );
    }

    private static <T> T requireType(
        Object value,
        Class<T> type
    )
    {
        if (!type.isInstance(value))
        {
            throw new AssertionError("expected " + type.getSimpleName() + " but got " + value);
        }
        return type.cast(value);
    }

    private static void require(
        boolean condition,
        String message
    )
    {
        if (!condition)
        {
            throw new AssertionError(message);
        }
    }

    private static void requireThrows(
        Runnable runnable,
        String message
    )
    {
        try
        {
            runnable.run();
        }
        catch (IllegalArgumentException expected)
        {
            return;
        }
        throw new AssertionError(message);
    }
}
