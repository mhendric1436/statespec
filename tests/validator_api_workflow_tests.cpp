#include "validator_test_support.hpp"

namespace
{

void validator_rejects_unknown_workflow_start_step()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          workflow OrderProcessing {
            version 1
            start missing_step
            step validate_order {
              expected_execution_time PT10S
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::UnknownReference),
        "validator should reject unknown workflow start step"
    );
}

void validator_rejects_incomplete_workflow_canonical_shape()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          workflow OrderProcessing {
            version 1
            start validate_order
            step validate_order {
              expected_execution_time PT10S
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "singleton"),
        "validator should require explicit workflow singleton"
    );
    require(
        has_error_message_containing(diagnostics, "expected_execution_time"),
        "validator should require explicit workflow expected_execution_time"
    );
    require(
        has_error_message_containing(diagnostics, "max_retries"),
        "validator should require explicit step max_retries"
    );
}

void validator_rejects_invalid_workflow_behavior_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          shape WorkflowState {
            attempt int
          }

          entity Order {
            key order_id
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              order_id string
            }
            state_machine {
              state Pending
              initial Pending
            }
          }

          workflow OrderProcessing {
            version 1
            on MissingTrigger
            input MissingInput
            state MissingState
            load MissingEntity by order_id as missing
            load Order by missing_key as order
            start validate_order
            step validate_order {
              expected_execution_time PT10S
              require feature_enabled(MissingFlag);
              emit MissingEvent;
              enqueue MissingQueue.Message;
              acquire lease MissingLease;
              start workflow MissingWorkflow;
              transition_to missing_step;
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "unknown workflow trigger reference"),
        "validator should reject unknown workflow trigger"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow input reference"),
        "validator should reject unknown workflow input"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow state reference"),
        "validator should reject unknown workflow state"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow load entity reference"),
        "validator should reject unknown workflow load entity"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow load key field reference"),
        "validator should reject workflow loads by non-key fields"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow emit target reference"),
        "validator should reject unknown workflow emit targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow enqueue target reference"),
        "validator should reject unknown workflow enqueue targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow lease target reference"),
        "validator should reject unknown workflow lease targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow start target reference"),
        "validator should reject unknown workflow start targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow transition target reference"),
        "validator should reject unknown workflow transition targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown feature flag reference"),
        "validator should validate feature flag references in workflow expressions"
    );
}

void validator_rejects_invalid_nested_workflow_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          feature_flag NewScheduler {
            type bool
            default false
            owner "platform"
          }

          entity Child {
            key child_id

            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }

            fields {
              created_at timestamp
              updated_at timestamp
              status string
              child_id uuid
              parent_id uuid
            }

            state_machine {
              state Pending
              state Deleted {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Pending
              terminal Deleted
              Pending -> Deleted
            }
          }

          workflow ParentWorkflow {
            version 1
            singleton false
            expected_execution_time PT5M
            start reserve_children
            child_set children_to_create {
              entity Child
              parent_field missing_parent
              desired_count feature_enabled(NewScheduler)
            }
            step reserve_children {
              expected_execution_time PT10S
              max_retries 2
              atomic {
                reserve child_set missing_child_set;
                when feature_enabled(MissingFlag) {
                  create child MissingChild {
                    key = generated_child_id;
                  }
                  transition_to missing_step;
                }
              }
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "unknown workflow child_set parent field"),
        "validator should reject invalid workflow child_set parent fields"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow child_set target reference"),
        "validator should reject unknown child_set statement targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow child entity target reference"),
        "validator should reject unknown create child entity targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow transition target reference"),
        "validator should reject nested transition targets"
    );
    require(
        has_error_message_containing(diagnostics, "unknown feature flag reference"),
        "validator should validate nested workflow expressions"
    );
}

void validator_accepts_valid_child_workflow_blocks()
{
    auto diagnostics = validate_text(R"sspec(
        system ChildWorkflowSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          entity Account {
            key tenant_id, account_id

            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }

            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              account_id uuid
              desired_task_count int
            }

            state_machine {
              state Active
              initial Active
            }
          }

          entity Task {
            key tenant_id, task_id

            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }

            relations {
              parent account_id: ref<Account> {
                kind: composition
                on_parent_delete: cascade
              }
            }

            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              task_id uuid
              account_id uuid
            }

            state_machine {
              state Pending
              state Active
              state Failed
              initial Pending
              Pending -> Active
              Pending -> Failed
            }
          }

          workflow TaskLifecycle {
            version 1
            singleton false
            expected_execution_time PT1M
            start run_task
            step run_task {
              expected_execution_time PT10S
              max_retries 1
            }
          }

          workflow AccountLifecycle {
            version 1
            singleton false
            expected_execution_time PT5M
            start run_account
            load Account by account_id as account
            child_workflow tasks {
              child_entity Task
              child_workflow TaskLifecycle
              child_id task_id uuid
              parent_ref account_id = account.account_id
              desired_count account.desired_task_count
              create {
                tenant_id: account.tenant_id
                task_id: task_id
                account_id: account.account_id
              }
              success when Task.status == Active
              failure when Task.status == Failed
            }
            step run_account {
              expected_execution_time PT10S
              max_retries 1
            }
          }
        }
    )sspec");

    require(
        !has_error_code(diagnostics, dc::UnknownReference),
        "validator should accept valid child_workflow references"
    );
    require(
        !has_error_code(diagnostics, dc::RequiredDeclaration),
        "validator should accept complete child_workflow declarations"
    );
}

void validator_rejects_invalid_child_workflow_blocks()
{
    auto diagnostics = validate_text(R"sspec(
        system ChildWorkflowSystem {
          feature_flag ExistingFlag {
            type bool
            default true
            scope system
          }

          entity Account {
            key account_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              account_id uuid
              desired_task_count int
            }
            state_machine {
              state Active
              initial Active
            }
          }

          entity Task {
            key task_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              task_id uuid
              account_id uuid
            }
            state_machine {
              state Pending
              initial Pending
            }
          }

          workflow AccountLifecycle {
            version 1
            singleton false
            expected_execution_time PT5M
            start run_account
            child_workflow tasks {
              child_entity Task
              child_workflow MissingWorkflow
              child_id task_id string
              parent_ref missing_parent = account.account_id
              desired_count feature_enabled(MissingFlag)
              create {
                task_id: task_id
                task_id: duplicate_task_id
                missing_field: account.account_id
              }
              success when feature_enabled(ExistingFlag)
              failure when feature_enabled(MissingFlag)
            }
            step run_account {
              expected_execution_time PT10S
              max_retries 1
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "unknown workflow child_workflow target"),
        "validator should reject unknown child workflow targets"
    );
    require(
        has_error_message_containing(diagnostics, "child_id field 'task_id' must have type string"),
        "validator should reject child_id type mismatches"
    );
    require(
        has_error_message_containing(
            diagnostics, "unknown workflow child_workflow parent_ref field"
        ),
        "validator should reject unknown parent_ref fields"
    );
    require(
        has_error_message_containing(
            diagnostics, "unknown workflow child_workflow parent_ref relation"
        ),
        "validator should require parent_ref to name a child parent relation"
    );
    require(
        has_error_message_containing(diagnostics, "unknown workflow child_workflow create field"),
        "validator should reject unknown create fields"
    );
    require(
        has_error_message_containing(diagnostics, "create assignment for parent_ref field"),
        "validator should require parent_ref assignment during child create"
    );
    require(
        has_error_message_containing(diagnostics, "duplicate declaration 'task_id'"),
        "validator should reject duplicate child create assignments"
    );
    require(
        has_error_message_containing(diagnostics, "unknown feature flag reference"),
        "validator should validate child_workflow expressions"
    );
}

void validator_rejects_incomplete_child_workflow_blocks()
{
    auto diagnostics = validate_text(R"sspec(
        system ChildWorkflowSystem {
          workflow AccountLifecycle {
            version 1
            singleton false
            expected_execution_time PT5M
            start run_account
            child_workflow tasks {
            }
            step run_account {
              expected_execution_time PT10S
              max_retries 1
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "child_entity"),
        "validator should require child_workflow child_entity"
    );
    require(
        has_error_message_containing(diagnostics, "child_workflow"),
        "validator should require child_workflow target workflow"
    );
    require(
        has_error_message_containing(diagnostics, "child_id"),
        "validator should require child_workflow child_id"
    );
    require(
        has_error_message_containing(diagnostics, "parent_ref"),
        "validator should require child_workflow parent_ref"
    );
    require(
        has_error_message_containing(diagnostics, "desired_count"),
        "validator should require child_workflow desired_count"
    );
    require(
        has_error_message_containing(diagnostics, "create"),
        "validator should require child_workflow create block"
    );
    require(
        has_error_message_containing(diagnostics, "success"),
        "validator should require child_workflow success condition"
    );
    require(
        has_error_message_containing(diagnostics, "failure"),
        "validator should require child_workflow failure condition"
    );
}

void validator_rejects_child_workflow_derived_name_collisions()
{
    auto diagnostics = validate_text(R"sspec(
        system ChildWorkflowSystem {
          entity Account {
            key account_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              account_id uuid
              desired_task_count int
            }
            state_machine {
              state Active
              initial Active
            }
          }

          entity Task {
            key task_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            relations {
              parent account_id: ref<Account> {
                kind: composition
                on_parent_delete: cascade
              }
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              task_id uuid
              account_id uuid
            }
            state_machine {
              state Pending
              state Active
              state Failed
              initial Pending
              Pending -> Active
              Pending -> Failed
            }
          }

          workflow TaskLifecycle {
            version 1
            singleton false
            expected_execution_time PT1M
            start run_task
            step run_task {
              expected_execution_time PT10S
              max_retries 1
            }
          }

          workflow AccountLifecycle {
            version 1
            singleton false
            expected_execution_time PT5M
            start generate_task_ids
            child_workflow tasks {
              child_entity Task
              child_workflow TaskLifecycle
              child_id task_id uuid
              parent_ref account_id = account.account_id
              desired_count account.desired_task_count
              create {
                task_id: task_id
                account_id: account.account_id
              }
              success when Task.status == Active
              failure when Task.status == Failed
            }
            child_workflow retry_tasks {
              child_entity Task
              child_workflow TaskLifecycle
              child_id task_id uuid
              parent_ref account_id = account.account_id
              desired_count account.desired_task_count
              create {
                task_id: task_id
                account_id: account.account_id
              }
              success when Task.status == Active
              failure when Task.status == Failed
            }
            step generate_task_ids {
              expected_execution_time PT10S
              max_retries 1
            }
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "derived step 'generate_task_ids'"),
        "validator should reject collisions with derived child_workflow step names"
    );
    require(
        has_error_message_containing(diagnostics, "derived bucket 'pending_task_ids'"),
        "validator should reject duplicate derived child_workflow bucket names"
    );
}

void validator_rejects_unknown_worker_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          worker WorkerA {
            singleton true
            lease MissingLease
            polls MissingQueue.Message
            executes MissingWorkflow
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::UnknownReference),
        "validator should reject unknown worker references"
    );
}

void validator_rejects_unknown_api_references()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input MissingRequest
            output MissingResponse
            error MissingError
            starts workflow MissingWorkflow
            enqueues MissingQueue.Message
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::UnknownReference),
        "validator should reject unknown API references"
    );
}

void validator_accepts_allowed_runtime_domain_dependencies()
{
    auto diagnostics = validate_text(R"sspec(
        system RuntimeDependencySystem {
          tenant scoped_by tenant_id
          system_tenant configured

          feature_flag NewScheduler {
            type bool
            default false
            scope tenant
          }

          log WorkflowDecision {
            level info
            event_name "workflow.decision"
            fields {
              tenant_id string
              order_id string
            }
          }

          queue OrderQueue {
            namespace orders
            channel order_starts
            visibility_timeout PT30S
            max_attempts 3
            message StartOrder {
              idempotency_key order_id
              payload {
                tenant_id string
                order_id string
              }
            }
          }

          lease WorkflowLease {
            resource "workflow"
            ttl PT30S
            renew_every PT10S
            holder worker_id
            fencing_token true
            max_ttl PT5M
          }

          workflow ChildWorkflow {
            version 1
            singleton false
            expected_execution_time PT1M
            start run_child
            step run_child {
              expected_execution_time PT10S
              max_retries 1
            }
          }

          workflow ParentWorkflow {
            version 1
            singleton false
            expected_execution_time PT1M
            start run_parent
            step run_parent {
              expected_execution_time PT10S
              max_retries 1
              require feature_enabled(NewScheduler);
              enqueue OrderQueue.StartOrder;
              acquire lease WorkflowLease;
              emit WorkflowDecision;
              start workflow ChildWorkflow;
            }
          }
        }
    )sspec");

    require(
        !has_error_code(diagnostics, dc::RuntimeDomainDependencyNotAllowed),
        "validator should allow workflow dependencies on supporting runtime domains"
    );
}

void validator_rejects_disallowed_runtime_domain_dependency()
{
    auto diagnostics = validate_text(R"sspec(
        system RuntimeDependencySystem {
          tenant scoped_by tenant_id
          system_tenant configured

          workflow WorkflowOnly {
            version 1
            singleton false
            expected_execution_time PT1M
            start run
            step run {
              expected_execution_time PT10S
              max_retries 1
            }
          }

          queue BrokenQueue {
            namespace orders
            channel order_starts
            visibility_timeout PT30S
            max_attempts 3
            dead_letter WorkflowOnly
            message StartOrder {
              idempotency_key order_id
              payload {
                tenant_id string
                order_id string
              }
            }
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::RuntimeDomainDependencyNotAllowed),
        "validator should reject queue-to-workflow runtime domain dependencies"
    );
}

void validator_rejects_api_with_multiple_primary_actions()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderResponse {
            accepted bool
          }
          queue OrderQueue {
            namespace orders
            channel order_starts
            visibility_timeout PT30S
            max_attempts 3
            message StartOrder {
              idempotency_key order_id
              payload {
                tenant_id string
                order_id string
              }
            }
          }
          workflow OrderProcessing {
            version 1
            singleton false
            expected_execution_time PT5M
            start process_order
            step process_order {
              expected_execution_time PT10S
              max_retries 1
            }
          }

          api StartOrder {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
            input StartOrderRequest
            output StartOrderResponse
            starts workflow OrderProcessing
            enqueues OrderQueue.StartOrder
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::ApiMultiplePrimaryActions),
        "validator should reject APIs with multiple primary actions"
    );
}

void validator_rejects_incomplete_api_canonical_shape()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          api StartOrderProcessing {
            method POST
            path "/v1/tenants/{tenantId}/orders/start"
          }
        }
    )sspec");

    require(
        has_error_message_containing(diagnostics, "input"),
        "validator should require input on mutating APIs"
    );
    require(
        has_error_message_containing(diagnostics, "output"),
        "validator should require output on non-DELETE APIs"
    );
}

void validator_rejects_tenant_scoped_api_path_without_tenant_identity()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          shape StartOrderRequest {
            tenant_id string
            order_id string
          }
          shape StartOrderResponse {
            accepted bool
          }

          api StartOrder {
            method POST
            path "/v1/orders/start"
            input StartOrderRequest
            output StartOrderResponse
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::TenantApiPathMissingTenantIdentity),
        "validator should reject tenant-scoped API paths without tenant identity"
    );
}

void validator_rejects_invalid_api_server_declarations()
{
    auto diagnostics = validate_text(R"sspec(
        system OrderSystem {
          api ExistingApi {
            method POST
            path "/v1/existing"
          }
          workflow NotAnApi {
            version 1
            start begin
            step begin {
              expected_execution_time PT1S
            }
          }
          api_server BrokenApiServer {
            serves MissingApi
            serves NotAnApi
            concurrency 0
          }
        }
    )sspec");

    require(
        has_error_code(diagnostics, dc::UnknownReference),
        "validator should reject unknown or non-API api_server serves targets"
    );
    require(
        has_error_code(diagnostics, dc::PositiveIntegerRequired),
        "validator should reject non-positive api_server concurrency"
    );
}

} // namespace

TEST_CASE("validator rejects unknown workflow start steps")
{
    validator_rejects_unknown_workflow_start_step();
}

TEST_CASE("validator rejects incomplete workflow canonical shape")
{
    validator_rejects_incomplete_workflow_canonical_shape();
}

TEST_CASE("validator rejects invalid workflow behavior references")
{
    validator_rejects_invalid_workflow_behavior_references();
}

TEST_CASE("validator rejects invalid nested workflow references")
{
    validator_rejects_invalid_nested_workflow_references();
}

TEST_CASE("validator accepts valid child workflow blocks")
{
    validator_accepts_valid_child_workflow_blocks();
}

TEST_CASE("validator rejects invalid child workflow blocks")
{
    validator_rejects_invalid_child_workflow_blocks();
}

TEST_CASE("validator rejects incomplete child workflow blocks")
{
    validator_rejects_incomplete_child_workflow_blocks();
}

TEST_CASE("validator rejects child workflow derived name collisions")
{
    validator_rejects_child_workflow_derived_name_collisions();
}

TEST_CASE("validator rejects unknown worker references")
{
    validator_rejects_unknown_worker_references();
}

TEST_CASE("validator rejects unknown API references")
{
    validator_rejects_unknown_api_references();
}

TEST_CASE("validator accepts allowed runtime domain dependencies")
{
    validator_accepts_allowed_runtime_domain_dependencies();
}

TEST_CASE("validator rejects disallowed runtime domain dependencies")
{
    validator_rejects_disallowed_runtime_domain_dependency();
}

TEST_CASE("validator rejects APIs with multiple primary actions")
{
    validator_rejects_api_with_multiple_primary_actions();
}

TEST_CASE("validator rejects incomplete API canonical shape")
{
    validator_rejects_incomplete_api_canonical_shape();
}

TEST_CASE("validator rejects tenant-scoped API path without tenant identity")
{
    validator_rejects_tenant_scoped_api_path_without_tenant_identity();
}

TEST_CASE("validator rejects invalid API server declarations")
{
    validator_rejects_invalid_api_server_declarations();
}
