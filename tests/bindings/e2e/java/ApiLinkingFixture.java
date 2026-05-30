package com.statespec.generated;

import com.statespec.backend.Backend;
import com.statespec.backend.Json;
import com.statespec.backend.memory.InMemoryBackend;
import com.statespec.generated.descriptors.types.ApiServerDescriptor;
import java.util.Map;
import java.util.Optional;

public final class ApiLinkingFixture
{
    private ApiLinkingFixture() {}

    private static final class LinkingHandler implements ApiHandlers.BusinessHandler
    {
        private final InMemoryBackend backend = new InMemoryBackend();

        public ApiResponse handleStartProvision(ApiRequestContext context) throws Exception
        {
            return recordRequest(context);
        }

        public ApiResponse handleReportProvisionReady(ApiRequestContext context) throws Exception
        {
            return recordRequest(context);
        }

        private ApiResponse recordRequest(ApiRequestContext context) throws Exception
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
            return new ApiResponse(202, Json.object(Map.of("linked", Json.bool(true))));
        }
    }

    public static void main(String[] args) throws Exception
    {
        ApiServerDescriptor descriptor =
            ApiServer.findApiServer("ProvisionApi")
                .orElseThrow(() -> new IllegalStateException("ProvisionApi descriptor not found"));
        ApiServer server = new ApiServer(
            descriptor,
            ApiHandlerRegistry.defaultHandler(new InMemoryBackend(), new LinkingHandler())
        );
        Optional<ApiResponse> response = server.handle(
            "ProvisionApi.StartProvision",
            new ApiRequestContext(
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
