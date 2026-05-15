package com.statespec.backend;

import com.statespec.backend.Backend.BackendException;
import com.statespec.backend.Backend.Transaction;
import java.util.Collections;
import java.util.Map;
import java.util.TreeMap;

public interface Observability
{
    enum LogLevel
    {
        DEBUG,
        INFO,
        WARN,
        ERROR
    }

    enum MetricKind
    {
        COUNTER,
        GAUGE,
        HISTOGRAM
    }

    record LogEvent(
        String name,
        LogLevel level,
        String eventName,
        Map<String,
            Json> fields
    )
    {
        public LogEvent
        {
            fields = Collections.unmodifiableMap(new TreeMap<>(fields));
        }
    }

    record MetricSample(
        String name,
        MetricKind kind,
        String backendName,
        double value,
        String unit,
        Map<String,
            Json> labels
    )
    {
        public MetricSample
        {
            labels = Collections.unmodifiableMap(new TreeMap<>(labels));
        }
    }

    void emitLog(
        Backend backend,
        LogEvent event
    ) throws BackendException;

    void emitLogTx(
        Transaction tx,
        LogEvent event
    ) throws BackendException;

    void recordMetric(
        Backend backend,
        MetricSample sample
    ) throws BackendException;

    void recordMetricTx(
        Transaction tx,
        MetricSample sample
    ) throws BackendException;
}
