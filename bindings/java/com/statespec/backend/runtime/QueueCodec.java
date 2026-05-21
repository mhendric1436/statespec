package com.statespec.backend.runtime;

import com.statespec.backend.Json;
import com.statespec.backend.Queue;
import java.util.Map;

final class QueueCodec
{
    private QueueCodec() {}

    static Json queueDefinitionToJson(Queue.QueueDefinition definition)
    {
        return Json.object(Map.of(
            "queue", Json.string(definition.queue()), "channel", Json.string(definition.channel()),
            "visibility_timeout", Codec.durationToJson(definition.visibilityTimeout()),
            "max_attempts", Json.integer(definition.maxAttempts()), "dead_letter_queue",
            Codec.optionalString(definition.deadLetterQueue()), "metadata",
            Json.string(definition.metadataJson())
        ));
    }

    static Queue.QueueDefinition queueDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Queue.QueueDefinition(
            Codec.stringFromJson(object.get("queue")), Codec.stringFromJson(object.get("channel")),
            Codec.durationFromJson(object.get("visibility_timeout")),
            Codec.intFromJson(object.get("max_attempts")),
            Codec.optionalStringFromJson(object.get("dead_letter_queue")),
            Codec.stringFromJson(object.get("metadata"))
        );
    }

    static Json queueMessageToJson(Queue.QueueMessageRecord message)
    {
        return Json.object(Map.of(
            "message_id", Json.string(message.messageId()), "queue", Json.string(message.queue()),
            "channel", Json.string(message.channel()), "status", Json.string(message.status()),
            "attempts", Json.integer(message.attempts()), "claimed_by",
            Codec.optionalString(message.claimedBy()), "claim_expires_at",
            Codec.optionalInstant(message.claimExpiresAt()), "payload",
            Json.string(message.payloadJson())
        ));
    }

    static Queue.QueueMessageRecord queueMessageFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Queue.QueueMessageRecord(
            Codec.stringFromJson(object.get("message_id")),
            Codec.stringFromJson(object.get("queue")), Codec.stringFromJson(object.get("channel")),
            Codec.stringFromJson(object.get("status")), Codec.longFromJson(object.get("attempts")),
            Codec.optionalStringFromJson(object.get("claimed_by")),
            Codec.optionalInstantFromJson(object.get("claim_expires_at")),
            Codec.stringFromJson(object.get("payload"))
        );
    }
}
