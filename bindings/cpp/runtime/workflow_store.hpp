#pragma once

#include "backend.hpp"
#include "codec.hpp"

namespace statespec::backend::runtime
{

class RuntimeWorkflowStore : public IWorkflowStore
{
  public:
    WorkflowDefinitionRegistration register_definition(
        IBackend& backend,
        const RegisterWorkflowDefinitionRequest& request
    ) override
    {
        ensure_collections(backend);
        auto tx = backend.begin();
        auto result = register_definitionTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    WorkflowDefinitionRegistration register_definitionTx(
        ITransaction& tx,
        const RegisterWorkflowDefinitionRequest& request
    ) override
    {
        const auto existing = inspect_definitionTx(
            tx, request.definition.workflow_name, request.definition.workflow_version
        );
        tx.put(
            kDefinitionsCollection,
            definition_key(request.definition.workflow_name, request.definition.workflow_version),
            detail::workflow_definition_to_json(request.definition)
        );
        return WorkflowDefinitionRegistration{request.definition, !existing.has_value()};
    }

    std::optional<WorkflowDefinition> inspect_definition(
        IBackend& backend,
        const std::string& workflow_name,
        std::int64_t workflow_version
    ) override
    {
        auto tx = backend.begin();
        auto result = inspect_definitionTx(*tx, workflow_name, workflow_version);
        backend.commit(*tx);
        return result;
    }

    std::optional<WorkflowDefinition> inspect_definitionTx(
        ITransaction& tx,
        const std::string& workflow_name,
        std::int64_t workflow_version
    ) override
    {
        const auto record =
            tx.get(kDefinitionsCollection, definition_key(workflow_name, workflow_version));
        if (!record.has_value())
        {
            return std::nullopt;
        }
        return detail::workflow_definition_from_json(record->document);
    }

    WorkflowExecutionRecord start(
        IBackend& backend,
        const StartWorkflowRequest& request
    ) override
    {
        ensure_collections(backend);
        auto tx = backend.begin();
        auto result = startTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    WorkflowExecutionRecord startTx(
        ITransaction& tx,
        const StartWorkflowRequest& request
    ) override
    {
        if (const auto existing = inspectTx(tx, request.workflow_execution_id);
            existing.has_value())
        {
            return *existing;
        }
        WorkflowExecutionRecord record;
        record.workflow_execution_id = request.workflow_execution_id;
        record.workflow_name = request.workflow_name;
        record.workflow_version = request.workflow_version;
        record.current_step = request.start_step;
        record.status = "Running";
        record.state = request.state;
        tx.put(
            kExecutionsCollection, record.workflow_execution_id,
            detail::workflow_execution_to_json(record)
        );
        return record;
    }

    std::vector<WorkflowExecutionRecord> claim_steps(
        IBackend& backend,
        const ClaimWorkflowStepRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = claim_stepsTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    std::vector<WorkflowExecutionRecord> claim_stepsTx(
        ITransaction& tx,
        const ClaimWorkflowStepRequest& request
    ) override
    {
        auto executions = all_executions(tx);
        std::vector<WorkflowExecutionRecord> claimed;
        for (auto& execution : executions)
        {
            if (claimed.size() >= request.max_steps)
            {
                break;
            }
            if (execution.workflow_execution_id != request.workflow_execution_id ||
                execution.workflow_name != request.workflow_name ||
                execution.workflow_version != request.workflow_version ||
                execution.status != "Running")
            {
                continue;
            }
            const auto visible =
                !execution.claimed_by.has_value() || (execution.claim_expires_at.has_value() &&
                                                      *execution.claim_expires_at <= request.now);
            if (!visible)
            {
                continue;
            }
            execution.claimed_by = request.worker;
            execution.claim_expires_at = request.now + request.lease_duration;
            ++execution.attempt;
            tx.put(
                kExecutionsCollection, execution.workflow_execution_id,
                detail::workflow_execution_to_json(execution)
            );
            claimed.push_back(execution);
        }
        return claimed;
    }

    WorkflowExecutionRecord keep_alive_step(
        IBackend& backend,
        const KeepAliveWorkflowStepRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = keep_alive_stepTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    WorkflowExecutionRecord keep_alive_stepTx(
        ITransaction& tx,
        const KeepAliveWorkflowStepRequest& request
    ) override
    {
        auto execution = require_execution(tx, request.workflow_execution_id);
        require_claim(execution, request.worker, request.current_step);
        execution.claim_expires_at = request.now + request.lease_duration;
        tx.put(
            kExecutionsCollection, execution.workflow_execution_id,
            detail::workflow_execution_to_json(execution)
        );
        return execution;
    }

    WorkflowExecutionRecord complete_step(
        IBackend& backend,
        const CompleteWorkflowStepRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = complete_stepTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    WorkflowExecutionRecord complete_stepTx(
        ITransaction& tx,
        const CompleteWorkflowStepRequest& request
    ) override
    {
        auto execution = require_execution(tx, request.workflow_execution_id);
        require_claim(execution, request.worker, request.completed_step);
        execution.state = request.state;
        execution.claimed_by.reset();
        execution.claim_expires_at.reset();
        if (request.next_step.has_value())
        {
            execution.current_step = *request.next_step;
            execution.status = "Running";
        }
        else
        {
            execution.status = "Completed";
        }
        tx.put(
            kExecutionsCollection, execution.workflow_execution_id,
            detail::workflow_execution_to_json(execution)
        );
        return execution;
    }

    WorkflowExecutionRecord fail_step(
        IBackend& backend,
        const FailWorkflowStepRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = fail_stepTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    WorkflowExecutionRecord fail_stepTx(
        ITransaction& tx,
        const FailWorkflowStepRequest& request
    ) override
    {
        auto execution = require_execution(tx, request.workflow_execution_id);
        require_claim(execution, request.worker, request.failed_step);
        execution.claimed_by.reset();
        execution.claim_expires_at.reset();
        execution.status = execution.attempt >= request.max_attempts ? "Failed" : "Running";
        tx.put(
            kExecutionsCollection, execution.workflow_execution_id,
            detail::workflow_execution_to_json(execution)
        );
        return execution;
    }

    WorkflowExecutionRecord cancel(
        IBackend& backend,
        const CancelWorkflowRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = cancelTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    WorkflowExecutionRecord cancelTx(
        ITransaction& tx,
        const CancelWorkflowRequest& request
    ) override
    {
        auto execution = require_execution(tx, request.workflow_execution_id);
        execution.status = "Canceled";
        execution.claimed_by.reset();
        execution.claim_expires_at.reset();
        tx.put(
            kExecutionsCollection, execution.workflow_execution_id,
            detail::workflow_execution_to_json(execution)
        );
        return execution;
    }

    std::optional<WorkflowExecutionRecord> inspect(
        IBackend& backend,
        const std::string& workflow_execution_id
    ) override
    {
        auto tx = backend.begin();
        auto result = inspectTx(*tx, workflow_execution_id);
        backend.commit(*tx);
        return result;
    }

    std::optional<WorkflowExecutionRecord> inspectTx(
        ITransaction& tx,
        const std::string& workflow_execution_id
    ) override
    {
        const auto record = tx.get(kExecutionsCollection, workflow_execution_id);
        if (!record.has_value())
        {
            return std::nullopt;
        }
        return detail::workflow_execution_from_json(record->document);
    }

  private:
    static constexpr const char* kDefinitionsCollection = runtime_collections::WorkflowDefinitions;
    static constexpr const char* kExecutionsCollection = runtime_collections::WorkflowExecutions;

    static void ensure_collections(IBackend& backend)
    {
        backend.ensure_collections(
            {CollectionDescriptor{
                 .name = kDefinitionsCollection,
                 .key_fields = {std::string{runtime_key_fields::WorkflowDefinition}}
             },
             CollectionDescriptor{
                 .name = kExecutionsCollection,
                 .key_fields = {std::string{runtime_key_fields::WorkflowExecutionId}}
             }}
        );
    }

    static std::string definition_key(
        const std::string& workflow_name,
        std::int64_t workflow_version
    )
    {
        return workflow_name + ":" + std::to_string(workflow_version);
    }

    static std::vector<WorkflowExecutionRecord> all_executions(ITransaction& tx)
    {
        std::vector<WorkflowExecutionRecord> executions;
        for (const auto& record : tx.query(kExecutionsCollection, Query::all()))
        {
            executions.push_back(detail::workflow_execution_from_json(record.document));
        }
        return executions;
    }

    static WorkflowExecutionRecord require_execution(
        ITransaction& tx,
        const std::string& workflow_execution_id
    )
    {
        RuntimeWorkflowStore store;
        auto execution = store.inspectTx(tx, workflow_execution_id);
        if (!execution.has_value())
        {
            throw BackendError("unknown workflow execution: " + workflow_execution_id);
        }
        return *execution;
    }

    static void require_claim(
        const WorkflowExecutionRecord& execution,
        const std::string& worker,
        const std::string& step
    )
    {
        if (!execution.claimed_by.has_value() || *execution.claimed_by != worker ||
            execution.current_step != step)
        {
            throw ConflictError(
                ConflictKind::WorkflowClaimConflict, "workflow step is not claimed by caller"
            );
        }
    }
};

} // namespace statespec::backend::runtime
