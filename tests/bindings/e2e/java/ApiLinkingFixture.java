package com.statespec.generated;

import com.statespec.backend.Backend;
import com.statespec.backend.Json;
import com.statespec.backend.memory.InMemoryBackend;
import java.util.Map;
import java.util.Optional;

public final class ApiLinkingFixture
{
    private ApiLinkingFixture() {}

    private static final class LinkingHandler implements Descriptors.ApiHandler
    {
        private final InMemoryBackend backend = new InMemoryBackend();

        @Override
        public Descriptors.ApiResponse handle(Descriptors.ApiRequestContext context)
            throws Exception
        {
            Backend.Transaction tx = backend.begin();
            backend.put(
                tx, "api_requests", "request-1",
                Json.object(Map.of(
                    "server", Json.string(context.serverName()), "api",
                    Json.string(context.apiName())
                ))
            );
            backend.commit(tx);
            return new Descriptors.ApiResponse(202, Json.object(Map.of("linked", Json.bool(true))));
        }
    }

    public static void main(String[] args) throws Exception
    {
        Descriptors.ApiServerDescriptor descriptor =
            ApiServer.findApiServer("ProvisionApi")
                .orElseThrow(() -> new IllegalStateException("ProvisionApi descriptor not found"));
        ApiServer server = new ApiServer(descriptor, new LinkingHandler());
        Optional<Descriptors.ApiResponse> response = server.handle(
            "ProvisionApi.StartProvision",
            new Descriptors.ApiRequestContext(
                "ProvisionApi", "StartProvision", Optional.of("POST"), Optional.of("/v1/provision"),
                Json.object(Map.of("tenant_id", Json.string("tenant-1")))
            )
        );
        if (response.isEmpty() || response.get().statusCode() != 202)
        {
            throw new IllegalStateException("generated API server did not dispatch linked handler");
        }
    }
}
