package com.statespec.backend;

import com.statespec.backend.Backend.BackendException;
import com.statespec.backend.Backend.Transaction;
import java.math.BigDecimal;
import java.util.Optional;

public interface FeatureFlag
{
    public enum Type
    {
        BOOL,
        STRING,
        INT,
        DECIMAL
    }

    public enum ScopeKind
    {
        SYSTEM,
        TENANT,
        USER,
        ENTITY
    }

    public sealed interface Value permits Value.BoolValue, Value.StringValue, Value.IntValue,
        Value.DecimalValue {

        Type type();

        default Optional<Boolean> asBool()
        {
            return Optional.empty();
        }

        default Optional<String> asString()
        {
            return Optional.empty();
        }

        default Optional<Long> asInt()
        {
            return Optional.empty();
        }

        default Optional<BigDecimal> asDecimal()
        {
            return Optional.empty();
        }

        record BoolValue(boolean value) implements Value
        {
            @Override public Type type()
            {
                return Type.BOOL;
            }

            @Override public Optional<Boolean> asBool()
            {
                return Optional.of(value);
            }
        }

        record StringValue(String value) implements Value
        {
            @Override public Type type()
            {
                return Type.STRING;
            }

            @Override public Optional<String> asString()
            {
                return Optional.of(value);
            }
        }

        record IntValue(long value) implements Value
        {
            @Override public Type type()
            {
                return Type.INT;
            }

            @Override public Optional<Long> asInt()
            {
                return Optional.of(value);
            }
        }

        record DecimalValue(BigDecimal value) implements Value
        {
            public DecimalValue(String value)
            {
                this(new BigDecimal(value));
            }

            @Override public Type type()
            {
                return Type.DECIMAL;
            }

            @Override public Optional<BigDecimal> asDecimal()
            {
                return Optional.of(value);
            }
        }
    }

    public record Definition(
        String name,
        Type type,
        Value defaultValue,
        ScopeKind scope,
        Optional<String> owner,
        Optional<String> description,
        Optional<String> expires
    )
    {
    }

    public record RegisterDefinitionResult(
        boolean registeredNew,
        Definition definition
    )
    {
    }

    public record EvaluationContext(
        Optional<String> tenantId,
        Optional<String> userId,
        Optional<String> entityType,
        Optional<String> entityId
    )
    {
    }

    public record EvaluationRequest(
        String name,
        EvaluationContext context
    )
    {
    }

    RegisterDefinitionResult registerDefinition(
        Backend backend,
        Definition definition
    ) throws BackendException;

    RegisterDefinitionResult registerDefinitionTx(
        Transaction tx,
        Definition definition
    ) throws BackendException;

    Optional<Definition> inspectDefinition(
        Backend backend,
        String name
    ) throws BackendException;

    Optional<Definition> inspectDefinitionTx(
        Transaction tx,
        String name
    ) throws BackendException;

    Value evaluate(
        Backend backend,
        EvaluationRequest request
    ) throws BackendException;

    Value evaluateTx(
        Transaction tx,
        EvaluationRequest request
    ) throws BackendException;
}
