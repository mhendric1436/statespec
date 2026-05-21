package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.FeatureFlag;
import com.statespec.backend.Json;
import com.statespec.backend.Lease;
import com.statespec.backend.Log;
import com.statespec.backend.Metric;
import com.statespec.backend.Queue;
import com.statespec.backend.Workflow;
import java.math.BigDecimal;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Optional;

final class InMemoryCodec
{
    private InMemoryCodec() {}

    static Json optionalString(Optional<String> value)
    {
        return value.map(Json::string).orElseGet(Json::nullValue);
    }

    static Optional<String> optionalStringFromJson(Json value)
    {
        if (value instanceof Json.NullValue)
        {
            return Optional.empty();
        }
        return Optional.of(stringFromJson(value));
    }

    static String stringFromJson(Json value)
    {
        return ((Json.StringValue)value).value();
    }

    static long longFromJson(Json value)
    {
        return ((Json.IntegerValue)value).value();
    }

    static int intFromJson(Json value)
    {
        return (int)longFromJson(value);
    }

    static double doubleFromJson(Json value)
    {
        if (value instanceof Json.DecimalValue decimal)
        {
            return decimal.value().doubleValue();
        }
        return longFromJson(value);
    }

    static boolean boolFromJson(Json value)
    {
        return ((Json.BooleanValue)value).value();
    }

    static Json durationToJson(Duration value)
    {
        return Json.integer(value.toNanos());
    }

    static Duration durationFromJson(Json value)
    {
        return Duration.ofNanos(longFromJson(value));
    }

    static Json optionalDuration(Optional<Duration> value)
    {
        return value.map(InMemoryCodec::durationToJson).orElseGet(Json::nullValue);
    }

    static Optional<Duration> optionalDurationFromJson(Json value)
    {
        if (value instanceof Json.NullValue)
        {
            return Optional.empty();
        }
        return Optional.of(durationFromJson(value));
    }

    static Json instantToJson(Instant value)
    {
        return Json.integer(value.toEpochMilli());
    }

    static Instant instantFromJson(Json value)
    {
        return Instant.ofEpochMilli(longFromJson(value));
    }

    static Json optionalInstant(Optional<Instant> value)
    {
        return value.map(InMemoryCodec::instantToJson).orElseGet(Json::nullValue);
    }

    static Optional<Instant> optionalInstantFromJson(Json value)
    {
        if (value instanceof Json.NullValue)
        {
            return Optional.empty();
        }
        return Optional.of(instantFromJson(value));
    }

    static Json featureFlagValueToJson(FeatureFlag.Value value)
    {
        Json encodedValue = switch (value)
        {
            case FeatureFlag.Value.BoolValue boolValue -> Json.bool(boolValue.value());
            case FeatureFlag.Value.StringValue stringValue -> Json.string(stringValue.value());
            case FeatureFlag.Value.IntValue intValue -> Json.integer(intValue.value());
            case FeatureFlag.Value.DecimalValue decimalValue -> Json.decimal(decimalValue.value());
        };
        return Json.object(Map.of("type", Json.string(value.type().name()), "value", encodedValue));
    }

    static FeatureFlag.Value featureFlagValueFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        var type = FeatureFlag.Type.valueOf(stringFromJson(object.get("type")));
        return switch (type)
        {
            case BOOL -> new FeatureFlag.Value.BoolValue(boolFromJson(object.get("value")));
            case STRING -> new FeatureFlag.Value.StringValue(stringFromJson(object.get("value")));
            case INT -> new FeatureFlag.Value.IntValue(longFromJson(object.get("value")));
            case DECIMAL ->
                new FeatureFlag.Value.DecimalValue(
                    ((Json.DecimalValue)object.get("value")).value()
                );
        };
    }

    static Json featureFlagDefinitionToJson(FeatureFlag.Definition definition)
    {
        return Json.object(Map.of(
            "name", Json.string(definition.name()), "type", Json.string(definition.type().name()),
            "default_value", featureFlagValueToJson(definition.defaultValue()), "scope",
            Json.string(definition.scope().name()), "owner", optionalString(definition.owner()),
            "description", optionalString(definition.description()), "expires",
            optionalString(definition.expires())
        ));
    }

    static FeatureFlag.Definition featureFlagDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new FeatureFlag.Definition(
            stringFromJson(object.get("name")),
            FeatureFlag.Type.valueOf(stringFromJson(object.get("type"))),
            featureFlagValueFromJson(object.get("default_value")),
            FeatureFlag.ScopeKind.valueOf(stringFromJson(object.get("scope"))),
            optionalStringFromJson(object.get("owner")),
            optionalStringFromJson(object.get("description")),
            optionalStringFromJson(object.get("expires"))
        );
    }

    static Json queueDefinitionToJson(Queue.QueueDefinition definition)
    {
        return Json.object(Map.of(
            "queue", Json.string(definition.queue()), "channel", Json.string(definition.channel()),
            "visibility_timeout", durationToJson(definition.visibilityTimeout()), "max_attempts",
            Json.integer(definition.maxAttempts()), "dead_letter_queue",
            optionalString(definition.deadLetterQueue()), "metadata",
            Json.string(definition.metadataJson())
        ));
    }

    static Queue.QueueDefinition queueDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Queue.QueueDefinition(
            stringFromJson(object.get("queue")), stringFromJson(object.get("channel")),
            durationFromJson(object.get("visibility_timeout")),
            intFromJson(object.get("max_attempts")),
            optionalStringFromJson(object.get("dead_letter_queue")),
            stringFromJson(object.get("metadata"))
        );
    }

    static Json queueMessageToJson(Queue.QueueMessageRecord message)
    {
        return Json.object(Map.of(
            "message_id", Json.string(message.messageId()), "queue", Json.string(message.queue()),
            "channel", Json.string(message.channel()), "status", Json.string(message.status()),
            "attempts", Json.integer(message.attempts()), "claimed_by",
            optionalString(message.claimedBy()), "claim_expires_at",
            optionalInstant(message.claimExpiresAt()), "payload", Json.string(message.payloadJson())
        ));
    }

    static Queue.QueueMessageRecord queueMessageFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Queue.QueueMessageRecord(
            stringFromJson(object.get("message_id")), stringFromJson(object.get("queue")),
            stringFromJson(object.get("channel")), stringFromJson(object.get("status")),
            longFromJson(object.get("attempts")), optionalStringFromJson(object.get("claimed_by")),
            optionalInstantFromJson(object.get("claim_expires_at")),
            stringFromJson(object.get("payload"))
        );
    }

    static Json leaseDefinitionIdToJson(Lease.LeaseDefinitionId id)
    {
        return Json.object(
            Map.of("name", Json.string(id.name()), "version", Json.integer(id.version()))
        );
    }

    static Lease.LeaseDefinitionId leaseDefinitionIdFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Lease.LeaseDefinitionId(
            stringFromJson(object.get("name")), longFromJson(object.get("version"))
        );
    }

    static Json leaseDefinitionToJson(Lease.LeaseDefinition definition)
    {
        return Json.object(Map.of(
            "id", leaseDefinitionIdToJson(definition.id()), "resource_pattern",
            Json.string(definition.resourcePattern()), "ttl", durationToJson(definition.ttl()),
            "renew_every", durationToJson(definition.renewEvery()), "max_ttl",
            optionalDuration(definition.maxTtl()), "fencing_token",
            Json.bool(definition.fencingToken())
        ));
    }

    static Lease.LeaseDefinition leaseDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Lease.LeaseDefinition(
            leaseDefinitionIdFromJson(object.get("id")),
            stringFromJson(object.get("resource_pattern")), durationFromJson(object.get("ttl")),
            durationFromJson(object.get("renew_every")),
            optionalDurationFromJson(object.get("max_ttl")),
            boolFromJson(object.get("fencing_token"))
        );
    }

    static Json leaseRecordToJson(Lease.LeaseRecord record)
    {
        return Json.object(Map.of(
            "definition_id", leaseDefinitionIdToJson(record.definitionId()), "resource",
            Json.string(record.resource()), "holder", optionalString(record.holder()), "expires_at",
            instantToJson(record.expiresAt()), "fencing_token", Json.integer(record.fencingToken())
        ));
    }

    static Lease.LeaseRecord leaseRecordFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Lease.LeaseRecord(
            leaseDefinitionIdFromJson(object.get("definition_id")),
            stringFromJson(object.get("resource")), optionalStringFromJson(object.get("holder")),
            instantFromJson(object.get("expires_at")), longFromJson(object.get("fencing_token"))
        );
    }

    static Json workflowDefinitionToJson(Workflow.WorkflowDefinition definition)
    {
        var steps = new ArrayList<Json>();
        for (var step : definition.steps())
        {
            steps.add(Json.object(Map.of(
                "name", Json.string(step.name()), "expected_execution_time",
                durationToJson(step.expectedExecutionTime()), "max_retries",
                Json.integer(step.maxRetries())
            )));
        }
        return Json.object(Map.of(
            "workflow_name", Json.string(definition.workflowName()), "workflow_version",
            Json.integer(definition.workflowVersion()), "start_step",
            Json.string(definition.startStep()), "expected_execution_time",
            durationToJson(definition.expectedExecutionTime()), "singleton",
            Json.bool(definition.singleton()), "steps", Json.array(steps), "metadata",
            Json.string(definition.metadataJson())
        ));
    }

    static Workflow.WorkflowDefinition workflowDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        var steps = new ArrayList<Workflow.WorkflowStepDefinition>();
        for (var stepJson : ((Json.ArrayValue)object.get("steps")).values())
        {
            var step = ((Json.ObjectValue)stepJson).values();
            steps.add(new Workflow.WorkflowStepDefinition(
                stringFromJson(step.get("name")),
                durationFromJson(step.get("expected_execution_time")),
                intFromJson(step.get("max_retries"))
            ));
        }
        return new Workflow.WorkflowDefinition(
            stringFromJson(object.get("workflow_name")),
            longFromJson(object.get("workflow_version")), stringFromJson(object.get("start_step")),
            durationFromJson(object.get("expected_execution_time")),
            boolFromJson(object.get("singleton")), List.copyOf(steps),
            stringFromJson(object.get("metadata"))
        );
    }

    static Json workflowExecutionToJson(Workflow.WorkflowExecutionRecord execution)
    {
        return Json.object(Map.of(
            "workflow_execution_id", Json.string(execution.workflowExecutionId()), "workflow_name",
            Json.string(execution.workflowName()), "workflow_version",
            Json.integer(execution.workflowVersion()), "current_step",
            Json.string(execution.currentStep()), "status", Json.string(execution.status()),
            "attempt", Json.integer(execution.attempt()), "claimed_by",
            optionalString(execution.claimedBy()), "claim_expires_at",
            optionalInstant(execution.claimExpiresAt()), "state", Json.string(execution.stateJson())
        ));
    }

    static Workflow.WorkflowExecutionRecord workflowExecutionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Workflow.WorkflowExecutionRecord(
            stringFromJson(object.get("workflow_execution_id")),
            stringFromJson(object.get("workflow_name")),
            longFromJson(object.get("workflow_version")),
            stringFromJson(object.get("current_step")), stringFromJson(object.get("status")),
            longFromJson(object.get("attempt")), optionalStringFromJson(object.get("claimed_by")),
            optionalInstantFromJson(object.get("claim_expires_at")),
            stringFromJson(object.get("state"))
        );
    }

    static Json fieldDescriptorToJson(Backend.FieldDescriptor field)
    {
        return Json.object(Map.of(
            "name", Json.string(field.name()), "type", Json.string(field.type().name()),
            "type_name", Json.string(field.typeName()), "required", Json.bool(field.required())
        ));
    }

    static Backend.FieldDescriptor fieldDescriptorFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Backend.FieldDescriptor(
            stringFromJson(object.get("name")),
            Backend.FieldType.valueOf(stringFromJson(object.get("type"))),
            stringFromJson(object.get("type_name")), boolFromJson(object.get("required"))
        );
    }

    static Json fieldsToJson(List<Backend.FieldDescriptor> fields)
    {
        var values = new ArrayList<Json>();
        for (var field : fields)
        {
            values.add(fieldDescriptorToJson(field));
        }
        return Json.array(values);
    }

    static List<Backend.FieldDescriptor> fieldsFromJson(Json value)
    {
        var fields = new ArrayList<Backend.FieldDescriptor>();
        for (var field : ((Json.ArrayValue)value).values())
        {
            fields.add(fieldDescriptorFromJson(field));
        }
        return List.copyOf(fields);
    }

    static Json logDefinitionToJson(Log.Definition definition)
    {
        return Json.object(Map.of(
            "name", Json.string(definition.name()), "level", Json.string(definition.level().name()),
            "event_name", Json.string(definition.eventName()), "fields",
            fieldsToJson(definition.fields())
        ));
    }

    static Log.Definition logDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Log.Definition(
            stringFromJson(object.get("name")),
            Log.Level.valueOf(stringFromJson(object.get("level"))),
            stringFromJson(object.get("event_name")), fieldsFromJson(object.get("fields"))
        );
    }

    static Json logEventToJson(Log.Event event)
    {
        return Json.object(Map.of(
            "name", Json.string(event.name()), "level", Json.string(event.level().name()),
            "event_name", Json.string(event.eventName()), "fields", Json.object(event.fields())
        ));
    }

    static Log.Event logEventFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Log.Event(
            stringFromJson(object.get("name")),
            Log.Level.valueOf(stringFromJson(object.get("level"))),
            stringFromJson(object.get("event_name")),
            ((Json.ObjectValue)object.get("fields")).values()
        );
    }

    static Json metricDefinitionToJson(Metric.Definition definition)
    {
        return Json.object(Map.of(
            "name", Json.string(definition.name()), "kind", Json.string(definition.kind().name()),
            "backend_name", Json.string(definition.backendName()), "unit",
            Json.string(definition.unit()), "labels", fieldsToJson(definition.labels())
        ));
    }

    static Metric.Definition metricDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Metric.Definition(
            stringFromJson(object.get("name")),
            Metric.Kind.valueOf(stringFromJson(object.get("kind"))),
            stringFromJson(object.get("backend_name")), stringFromJson(object.get("unit")),
            fieldsFromJson(object.get("labels"))
        );
    }

    static Json metricSampleToJson(Metric.Sample sample)
    {
        return Json.object(Map.of(
            "name", Json.string(sample.name()), "kind", Json.string(sample.kind().name()),
            "backend_name", Json.string(sample.backendName()), "value",
            Json.decimal(BigDecimal.valueOf(sample.value())), "unit", Json.string(sample.unit()),
            "labels", Json.object(sample.labels())
        ));
    }

    static Metric.Sample metricSampleFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Metric.Sample(
            stringFromJson(object.get("name")),
            Metric.Kind.valueOf(stringFromJson(object.get("kind"))),
            stringFromJson(object.get("backend_name")), doubleFromJson(object.get("value")),
            stringFromJson(object.get("unit")), ((Json.ObjectValue)object.get("labels")).values()
        );
    }
}
