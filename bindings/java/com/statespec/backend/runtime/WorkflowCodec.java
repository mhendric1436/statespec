package com.statespec.backend.runtime;

import com.statespec.backend.Json;
import com.statespec.backend.Workflow;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

final class WorkflowCodec
{
    private WorkflowCodec() {}

    static Json workflowDefinitionToJson(Workflow.WorkflowDefinition definition)
    {
        var steps = new ArrayList<Json>();
        for (var step : definition.steps())
        {
            steps.add(Json.object(Map.of(
                "name", Json.string(step.name()), "expected_execution_time",
                Codec.durationToJson(step.expectedExecutionTime()), "max_retries",
                Json.integer(step.maxRetries())
            )));
        }
        return Json.object(Map.of(
            "workflow_name", Json.string(definition.workflowName()), "workflow_version",
            Json.integer(definition.workflowVersion()), "start_step",
            Json.string(definition.startStep()), "expected_execution_time",
            Codec.durationToJson(definition.expectedExecutionTime()), "singleton",
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
                Codec.stringFromJson(step.get("name")),
                Codec.durationFromJson(step.get("expected_execution_time")),
                Codec.intFromJson(step.get("max_retries"))
            ));
        }
        return new Workflow.WorkflowDefinition(
            Codec.stringFromJson(object.get("workflow_name")),
            Codec.longFromJson(object.get("workflow_version")),
            Codec.stringFromJson(object.get("start_step")),
            Codec.durationFromJson(object.get("expected_execution_time")),
            Codec.boolFromJson(object.get("singleton")), List.copyOf(steps),
            Codec.stringFromJson(object.get("metadata"))
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
            Codec.optionalString(execution.claimedBy()), "claim_token",
            Codec.optionalString(execution.claimToken()), "claim_expires_at",
            Codec.optionalInstant(execution.claimExpiresAt()), "state",
            Json.string(execution.stateJson())
        ));
    }

    static Workflow.WorkflowExecutionRecord workflowExecutionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Workflow.WorkflowExecutionRecord(
            Codec.stringFromJson(object.get("workflow_execution_id")),
            Codec.stringFromJson(object.get("workflow_name")),
            Codec.longFromJson(object.get("workflow_version")),
            Codec.stringFromJson(object.get("current_step")),
            Codec.stringFromJson(object.get("status")), Codec.longFromJson(object.get("attempt")),
            Codec.optionalStringFromJson(object.get("claimed_by")),
            Codec.optionalStringFromJson(object.get("claim_token")),
            Codec.optionalInstantFromJson(object.get("claim_expires_at")),
            Codec.stringFromJson(object.get("state"))
        );
    }

    static Json workflowHeartbeatToJson(Workflow.WorkflowHeartbeatRecord heartbeat)
    {
        return Json.object(Map.of(
            "workflow_execution_id", Json.string(heartbeat.workflowExecutionId()), "claim_token",
            Json.string(heartbeat.claimToken()), "worker", Json.string(heartbeat.worker()),
            "current_step", Json.string(heartbeat.currentStep()), "claim_expires_at",
            Codec.instantToJson(heartbeat.claimExpiresAt())
        ));
    }

    static Workflow.WorkflowHeartbeatRecord workflowHeartbeatFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Workflow.WorkflowHeartbeatRecord(
            Codec.stringFromJson(object.get("workflow_execution_id")),
            Codec.stringFromJson(object.get("claim_token")),
            Codec.stringFromJson(object.get("worker")),
            Codec.stringFromJson(object.get("current_step")),
            Codec.instantFromJson(object.get("claim_expires_at"))
        );
    }
}
