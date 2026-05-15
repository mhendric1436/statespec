package com.statespec.backend;

import com.statespec.backend.Backend.BackendException;
import com.statespec.backend.Backend.Transaction;
import java.util.Collections;
import java.util.Map;
import java.util.TreeMap;

public interface Log
{
    enum Level
    {
        DEBUG,
        INFO,
        WARN,
        ERROR
    }

    record Event(
        String name,
        Level level,
        String eventName,
        Map<String,
            Json> fields
    )
    {
        public Event
        {
            fields = Collections.unmodifiableMap(new TreeMap<>(fields));
        }
    }

    void emit(
        Backend backend,
        Event event
    ) throws BackendException;

    void emitTx(
        Transaction tx,
        Event event
    ) throws BackendException;
}
