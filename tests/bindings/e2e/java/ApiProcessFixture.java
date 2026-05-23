package com.statespec.generated;

import com.statespec.backend.memory.InMemoryBackend;

public final class ApiProcessFixture
{
    private ApiProcessFixture() {}

    public static void main(String[] args) throws Exception
    {
        var backend = new InMemoryBackend();
        var handler = ApiHandlerRegistry.defaultHandler(backend);
        var transport = new ApiTransport.LocalBlocking();
        var config = ApiProcess.Config.allServers();
        if (config.serverNames().size() != 2)
        {
            throw new IllegalStateException("expected two generated API server descriptors");
        }

        var process = new ApiProcess(config, backend, handler, transport);
        if (process.applications().size() != 2)
        {
            throw new IllegalStateException("expected two generated API applications");
        }

        final int[] status = {1};
        var runner = new Thread(() -> {
            try
            {
                status[0] = process.run();
            }
            catch (Exception error)
            {
                throw new RuntimeException(error);
            }
        }, "statespec-api-process-fixture");
        runner.start();
        process.requestStop();
        runner.join(2000);

        if (runner.isAlive())
        {
            throw new IllegalStateException("API process did not stop");
        }
        if (status[0] != 0)
        {
            throw new IllegalStateException("API process did not stop cleanly");
        }
    }
}
