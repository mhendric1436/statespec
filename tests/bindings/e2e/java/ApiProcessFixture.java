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

        var notStarted = new ApiProcess(config, backend, handler, new ApiTransport.LocalBlocking());
        notStarted.requestStop();
        try
        {
            notStarted.join();
            throw new IllegalStateException("join before start should fail");
        }
        catch (IllegalStateException expected)
        {
        }

        process.start();
        if (!process.isRunning())
        {
            throw new IllegalStateException("API process did not report running");
        }
        try
        {
            process.start();
            throw new IllegalStateException("API process allowed double start");
        }
        catch (IllegalStateException expected)
        {
        }
        process.requestStop();
        final int[] status = {1};
        final Throwable[] failure = {null};
        var joiner = new Thread(() -> {
            try
            {
                status[0] = process.join();
            }
            catch (Exception error)
            {
                failure[0] = error;
            }
        }, "statespec-api-process-joiner");
        joiner.start();
        joiner.join(2000);

        if (joiner.isAlive())
        {
            throw new IllegalStateException("API process did not stop");
        }
        if (failure[0] != null)
        {
            throw new RuntimeException(failure[0]);
        }
        if (status[0] != 0)
        {
            throw new IllegalStateException("API process did not stop cleanly");
        }
        if (process.isRunning())
        {
            throw new IllegalStateException("API process still reports running after join");
        }
    }
}
