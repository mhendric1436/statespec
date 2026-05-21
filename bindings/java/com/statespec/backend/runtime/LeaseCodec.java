package com.statespec.backend.runtime;

import com.statespec.backend.Json;
import com.statespec.backend.Lease;
import java.util.Map;

final class LeaseCodec
{
    private LeaseCodec() {}

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
            Codec.stringFromJson(object.get("name")), Codec.longFromJson(object.get("version"))
        );
    }

    static Json leaseDefinitionToJson(Lease.LeaseDefinition definition)
    {
        return Json.object(Map.of(
            "id", leaseDefinitionIdToJson(definition.id()), "resource_pattern",
            Json.string(definition.resourcePattern()), "ttl",
            Codec.durationToJson(definition.ttl()), "renew_every",
            Codec.durationToJson(definition.renewEvery()), "max_ttl",
            Codec.optionalDuration(definition.maxTtl()), "fencing_token",
            Json.bool(definition.fencingToken())
        ));
    }

    static Lease.LeaseDefinition leaseDefinitionFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Lease.LeaseDefinition(
            leaseDefinitionIdFromJson(object.get("id")),
            Codec.stringFromJson(object.get("resource_pattern")),
            Codec.durationFromJson(object.get("ttl")),
            Codec.durationFromJson(object.get("renew_every")),
            Codec.optionalDurationFromJson(object.get("max_ttl")),
            Codec.boolFromJson(object.get("fencing_token"))
        );
    }

    static Json leaseRecordToJson(Lease.LeaseRecord record)
    {
        return Json.object(Map.of(
            "definition_id", leaseDefinitionIdToJson(record.definitionId()), "resource",
            Json.string(record.resource()), "holder", Codec.optionalString(record.holder()),
            "expires_at", Codec.instantToJson(record.expiresAt()), "fencing_token",
            Json.integer(record.fencingToken())
        ));
    }

    static Lease.LeaseRecord leaseRecordFromJson(Json value)
    {
        var object = ((Json.ObjectValue)value).values();
        return new Lease.LeaseRecord(
            leaseDefinitionIdFromJson(object.get("definition_id")),
            Codec.stringFromJson(object.get("resource")),
            Codec.optionalStringFromJson(object.get("holder")),
            Codec.instantFromJson(object.get("expires_at")),
            Codec.longFromJson(object.get("fencing_token"))
        );
    }
}
