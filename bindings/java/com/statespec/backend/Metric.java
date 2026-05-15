package com.statespec.backend;

import com.statespec.backend.Backend.BackendException;
import com.statespec.backend.Backend.Transaction;
import java.util.Collections;
import java.util.Map;
import java.util.TreeMap;

public interface Metric
{
    enum Kind
    {
        COUNTER,
        GAUGE,
        HISTOGRAM
    }

    record Sample(
        String name,
        Kind kind,
        String backendName,
        double value,
        String unit,
        Map<String,
            Json> labels
    )
    {
        public Sample
        {
            labels = Collections.unmodifiableMap(new TreeMap<>(labels));
        }
    }

    void record(
        Backend backend,
        Sample sample
    ) throws BackendException;

    void recordTx(
        Transaction tx,
        Sample sample
    ) throws BackendException;
}
