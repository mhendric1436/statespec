package com.statespec.generated;

import com.statespec.backend.memory.InMemoryBackend;

public final class WorkerProcessFixture
{
    private WorkerProcessFixture() {}

    public static void main(String[] args) throws Exception
    {
        InMemoryBackend backend = new InMemoryBackend();
        WorkerRuntime runtime = new WorkerRuntime(backend);
        WorkerProcess process = new WorkerProcess(runtime);

        try
        {
            process.join();
            throw new IllegalStateException("joining a WorkerProcess before start should fail");
        }
        catch (IllegalStateException expected)
        {
        }
        process.requestStop();

        process.start();
        try
        {
            process.start();
            throw new IllegalStateException("starting WorkerProcess twice should fail");
        }
        catch (IllegalStateException expected)
        {
        }
        Thread.sleep(10);
        if (!process.isRunning())
        {
            throw new IllegalStateException("started WorkerProcess should report running");
        }

        process.requestStop();
        if (process.join() != 0)
        {
            throw new IllegalStateException("stopped WorkerProcess should join cleanly");
        }
    }
}
