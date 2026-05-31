#!/bin/sh

binding_test_init() {
    CLI="$1"
    SCRIPT_PATH="$2"
    TMPDIR="$(mktemp -d)"
    cleanup() {
        rm -rf "$TMPDIR"
    }
    trap cleanup EXIT

    SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$SCRIPT_PATH")" && pwd)"
    TESTS_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
    . "$TESTS_DIR/cli/common.sh"
    SPEC="$TESTS_DIR/fixtures/bindings-full.sspec"
}

generate_binding_fixture() {
    lang="$1"
    out_dir="$2"
    expected_generated_file="$3"
    run_expect_status 0 "$CLI" generate bindings --lang "$lang" "$SPEC" --out "$out_dir"
    assert_output_contains "generated $out_dir/$expected_generated_file"
}

assert_generated_files_exist() {
    root="$1"
    while IFS= read -r path; do
        if [ -n "$path" ]; then
            assert_file_exists "$root/$path"
        fi
    done
}

assert_generated_files_absent() {
    root="$1"
    while IFS= read -r path; do
        if [ -n "$path" ]; then
            assert_file_not_exists "$root/$path"
        fi
    done
}

assert_generated_tree_has_no_format_guards() {
    root="$1"
    extension="$2"
    assert_tree_files_not_contains "$root" "*.$extension" "// clang-format off"
    assert_tree_files_not_contains "$root" "*.$extension" "// clang-format on"
}

assert_cpp_workflow_transaction_regression() {
    root="$1"
    assert_file_contains "$root/worker/main.cpp" "WorkflowStepInvokerMap invokers;"
    assert_file_contains "$root/worker/main.cpp" "register_workflow_step_invokers(invokers,"
    assert_file_not_contains "$root/worker/main.cpp" "DefaultWorkflowStepHandlerBundle"
    assert_file_contains "$root/worker/workflow_step_types.hpp" "using WorkflowStepInvoker = std::function<"
    assert_file_contains "$root/worker/workflow_step_types.hpp" "statespec::backend::IBackend& backend"
    assert_file_contains "$root/worker/workflow_step_types.hpp" "statespec::backend::ITransaction& tx"
    assert_file_contains "$root/worker/workflows/provision_service/handlers.hpp" "WorkflowStepResult handle_validate_request"
    assert_file_contains "$root/worker/workflows/provision_service/registry.hpp" "return handler.handle_validate_request(tx, context);"
    assert_file_contains "$root/worker/workflows/provision_service/registry.hpp" "return handler.handle_create_remote_service(tx, context);"
    assert_file_contains "$root/worker/workflows/provision_service/registry.hpp" "return handler.handle_wait_for_remote_service(tx, context);"
    assert_file_contains "$root/worker/workflow_runner.hpp" "workflow_store_.keep_alive_step"
    assert_file_occurrence_count "$root/worker/workflow_runner.hpp" "workflow_store_.keep_alive_step" 1
    assert_file_contains "$root/worker/workflow_runner.hpp" "class PeriodicWorkflowKeepAlive"
    assert_file_contains "$root/worker/workflow_runner.hpp" "keep_alive.start()"
    assert_file_contains "$root/worker/workflow_runner.hpp" "keep_alive.stop()"
    assert_file_contains "$root/worker/workflow_runner.hpp" "auto tx = backend_.begin()"
    assert_file_contains "$root/worker/workflow_runner.hpp" "const auto current = revalidate_claimed_step(*tx, record)"
    assert_file_contains "$root/worker/workflow_runner.hpp" "result = invoker->second(backend_, *tx, context)"
    assert_file_contains "$root/worker/workflow_runner.hpp" "workflow_store_.complete_stepTx"
    assert_file_contains "$root/worker/workflow_runner.hpp" "workflow_store_.fail_stepTx"
    assert_file_contains "$root/worker/workflow_runner.hpp" "workflow_store_.cancelTx"
    assert_file_contains "$root/worker/workflow_runner.hpp" "if (tx->is_open())"
    assert_file_contains "$root/worker/workflow_runner.hpp" "unknown generated workflow step handler: \" + key"
    assert_file_not_contains "$root/worker/workflow_runner.hpp" "WorkflowNextStepResolver"
    assert_file_not_contains "$root/worker/workflow_runner.hpp" "if (!next_step.has_value())"
    assert_file_not_exists "$root/worker/workflows/provision_service.hpp"
}

assert_go_workflow_result_regression() {
    root="$1"
    assert_file_contains "$root/worker/cmd/worker/main.go" "invokers := worker.WorkflowStepInvokerMap{}"
    assert_file_contains "$root/worker/cmd/worker/main.go" "worker.RegisterProvisionServiceWorkflowStepInvokers(invokers,"
    assert_file_not_contains "$root/worker/cmd/worker/main.go" "worker.NewDefaultWorkflowStepHandlerBundle"
    assert_file_contains "$root/worker/backend/workflows/provision_service/handlers.go" "HandleValidateRequest(context.Context, common.Transaction, workflowcontext.WorkflowStepHandlerContext) (workflowcontext.WorkflowStepResult, error)"
    assert_file_contains "$root/worker/backend/workflows/provision_service/registry.go" "return handler.HandleValidateRequest(ctx, tx, stepContext)"
    assert_file_contains "$root/worker/backend/workflows/provision_service/registry.go" "return handler.HandleCreateRemoteService(ctx, tx, stepContext)"
    assert_file_contains "$root/worker/backend/workflows/provision_service/registry.go" "return handler.HandleWaitForRemoteService(ctx, tx, stepContext)"
    assert_file_contains "$root/worker/backend/workflow_runner.go" "invoker, handled := runner.Invokers[stepKey]"
    assert_file_contains "$root/worker/backend/workflow_runner.go" "unknown generated workflow step handler: %s"
    assert_file_contains "$root/worker/backend/workflow_runner.go" "case WorkflowStepFail:"
    assert_file_contains "$root/worker/backend/workflow_runner.go" "case WorkflowStepCancel:"
    assert_file_occurrence_count "$root/worker/backend/workflow_runner.go" "runner.WorkflowStore.KeepAliveStep" 1
    assert_file_contains "$root/worker/backend/workflow_runner.go" "tx, err := runner.Backend.Begin(ctx)"
    assert_file_contains "$root/worker/backend/workflow_runner.go" "runner.revalidateClaimedStep(ctx, tx, record)"
    assert_file_contains "$root/worker/backend/workflow_runner.go" "result, err = invoker(ctx, runner.Backend, tx, stepContext)"
    assert_file_contains "$root/worker/backend/workflow_runner.go" "CompleteStepTx(ctx"
    assert_file_contains "$root/worker/backend/workflow_runner.go" "FailStepTx(ctx, tx"
    assert_file_contains "$root/worker/backend/workflow_runner.go" "CancelTx(ctx, tx"
    assert_file_contains "$root/worker/backend/workflow_runner.go" "if !committed && tx.IsOpen()"
    assert_file_not_contains "$root/worker/backend/workflow_runner.go" "NextProvisionServiceStep"
    assert_file_not_contains "$root/worker/backend/workflow_runner.go" "case record.WorkflowName"
    assert_file_not_exists "$root/worker/backend/workflows/provision_service.go"
}

assert_java_workflow_result_regression() {
    root="$1"
    assert_file_contains "$root/worker/com/statespec/generated/WorkerMain.java" "new java.util.LinkedHashMap<String, WorkflowStepHandlers.WorkflowStepInvoker>()"
    assert_file_contains "$root/worker/com/statespec/generated/WorkerMain.java" "WorkerRegistry.registerProvisionServiceWorkflowStepInvokers(invokers,"
    assert_file_not_contains "$root/worker/com/statespec/generated/WorkerMain.java" "WorkflowStepHandlers.DefaultHandlerBundle"
    assert_file_contains "$root/worker/com/statespec/generated/workflows/provision_service/Handlers.java" "WorkflowStepHandlers.WorkflowStepResult handleValidateRequest(Backend.Transaction tx, WorkflowStepHandlers.Context context)"
    assert_file_contains "$root/worker/com/statespec/generated/workflows/provision_service/Registry.java" "return handler.handleValidateRequest(tx, context);"
    assert_file_contains "$root/worker/com/statespec/generated/workflows/provision_service/Registry.java" "return handler.handleCreateRemoteService(tx, context);"
    assert_file_contains "$root/worker/com/statespec/generated/workflows/provision_service/Registry.java" "return handler.handleWaitForRemoteService(tx, context);"
    assert_file_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "WorkflowStepHandlers.WorkflowStepInvoker invoker"
    assert_file_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "unknown generated workflow step handler: \" + stepKey"
    assert_file_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "WorkflowStepResultKind.FAIL"
    assert_file_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "WorkflowStepResultKind.CANCEL"
    assert_file_occurrence_count "$root/worker/com/statespec/generated/WorkflowRunner.java" "workflowStore.keepAliveStep" 1
    assert_file_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "Backend.Transaction tx = backend.begin();"
    assert_file_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "revalidateClaimedStep(tx, record)"
    assert_file_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "result = invoker.invoke(backend, tx, context);"
    assert_file_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "workflowStore.completeStepTx"
    assert_file_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "workflowStore.failStepTx"
    assert_file_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "workflowStore.cancelTx"
    assert_file_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "if (!committed && tx.isOpen())"
    assert_file_not_contains "$root/worker/com/statespec/generated/WorkflowRunner.java" "nextStep ="
    assert_file_not_exists "$root/worker/com/statespec/generated/workflows/ProvisionServiceWorkerModule.java"
}

assert_rust_workflow_result_regression() {
    root="$1"
    assert_file_contains "$root/worker/main.rs" "let mut workflow_step_invokers = WorkflowStepInvokerMap::new();"
    assert_file_contains "$root/worker/main.rs" "workflow_provision_service_registry::register_workflow_step_invokers::<InMemoryBackend>"
    assert_file_not_contains "$root/worker/main.rs" "DefaultWorkflowStepHandlerBundle"
    assert_file_contains "$root/worker/workflows/provision_service/handlers.rs" "BackendResult<WorkflowStepResult>"
    assert_file_contains "$root/worker/workflows/provision_service/registry.rs" "handler.handle_validate_request(tx, context)"
    assert_file_contains "$root/worker/workflows/provision_service/registry.rs" "handler.handle_create_remote_service(tx, context)"
    assert_file_contains "$root/worker/workflows/provision_service/registry.rs" "handler.handle_wait_for_remote_service(tx, context)"
    assert_file_contains "$root/worker/workflow_runner.rs" "self.workflow_step_invokers.get(&key)"
    assert_file_contains "$root/worker/workflow_runner.rs" "unknown generated workflow step handler: {}"
    assert_file_contains "$root/worker/workflow_runner.rs" "WorkflowStepResult::Complete { next_step }"
    assert_file_contains "$root/worker/workflow_runner.rs" "WorkflowStepResult::Fail { reason }"
    assert_file_contains "$root/worker/workflow_runner.rs" "WorkflowStepResult::Cancel { reason }"
    assert_file_occurrence_count "$root/worker/workflow_runner.rs" "self.workflow_store.keep_alive_step" 1
    assert_file_contains "$root/worker/workflow_runner.rs" "let mut tx = self.backend.begin()?"
    assert_file_contains "$root/worker/workflow_runner.rs" "self.revalidate_claimed_step(tx, &claimed)"
    assert_file_contains "$root/worker/workflow_runner.rs" "Some(invoker) => invoker(self.backend, tx, &context)"
    assert_file_contains "$root/worker/workflow_runner.rs" ".complete_step_tx("
    assert_file_contains "$root/worker/workflow_runner.rs" ".fail_step_tx("
    assert_file_contains "$root/worker/workflow_runner.rs" ".cancel_tx("
    assert_file_contains "$root/worker/workflow_runner.rs" "if tx.is_open()"
    assert_file_not_contains "$root/worker/workflow_runner.rs" "next_step ="
    assert_file_not_exists "$root/worker/workflows/provision_service.rs"
}
