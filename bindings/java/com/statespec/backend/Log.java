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

    record Definition(
        String name,
        Level level,
        String eventName,
        java.util.List<Backend.FieldDescriptor> fields
    )
    {
        public Definition
        {
            fields = java.util.List.copyOf(fields);
        }
    }

    record DefinitionRegistration(
        boolean registeredNew,
        Definition definition
    )
    {
    }

    DefinitionRegistration registerDefinition(
        Backend backend,
        Definition definition
    ) throws BackendException;

    DefinitionRegistration registerDefinitionTx(
        Transaction tx,
        Definition definition
    ) throws BackendException;

    void emit(
        Backend backend,
        Event event
    ) throws BackendException;

    void emitTx(
        Transaction tx,
        Event event
    ) throws BackendException;
}
