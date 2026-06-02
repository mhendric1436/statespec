#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

UNFORMATTED="$TMPDIR/unformatted.sspec"
FORMATTED="$TMPDIR/formatted.sspec"
EXPECTED="$TMPDIR/expected.sspec"
GC_UNFORMATTED="$TMPDIR/gc-unformatted.sspec"
GC_FORMATTED="$TMPDIR/gc-formatted.sspec"
GC_EXPECTED="$TMPDIR/gc-expected.sspec"
INCLUDE_UNFORMATTED="$TMPDIR/include-unformatted.sspec"
INCLUDE_FORMATTED="$TMPDIR/include-formatted.sspec"
INCLUDE_EXPECTED="$TMPDIR/include-expected.sspec"
API_SERVER_UNFORMATTED="$TMPDIR/api-server-unformatted.sspec"
API_SERVER_FORMATTED="$TMPDIR/api-server-formatted.sspec"
API_SERVER_EXPECTED="$TMPDIR/api-server-expected.sspec"
AST_UNFORMATTED="$TMPDIR/ast-unformatted.sspec"
AST_FORMATTED="$TMPDIR/ast-formatted.sspec"
AST_EXPECTED="$TMPDIR/ast-expected.sspec"
PARITY_FIXTURE="testdata/parity/kitchen-sink.sspec"

cat > "$UNFORMATTED" <<'SSPEC'
statespec 0.1; system Demo { feature_flag NewScheduler { type bool default false scope tenant owner "platform" description "New scheduler" expires "2026-12-31" } }
SSPEC

cat > "$EXPECTED" <<'SSPEC'
statespec 0.1;
system Demo {
  feature_flag NewScheduler {
    type bool
    default false
    scope tenant
    owner "platform"
    description "New scheduler"
    expires "2026-12-31"
  }
}
SSPEC

"$CLI" fmt "$UNFORMATTED" > "$FORMATTED"
diff -u "$EXPECTED" "$FORMATTED"

"$CLI" fmt --check "$FORMATTED"

set +e
"$CLI" fmt --check "$UNFORMATTED" > "$TMPDIR/check-output.txt" 2>&1
STATUS="$?"
set -e

if [ "$STATUS" -eq 0 ]; then
    echo "expected fmt --check to reject unformatted file" >&2
    exit 1
fi

grep -F "is not formatted" "$TMPDIR/check-output.txt" >/dev/null

cat > "$GC_UNFORMATTED" <<'SSPEC'
system Demo { entity Order { key order_id; fields { order_id string; created_at timestamp; updated_at timestamp; status string; } state_machine { state Creating; state Deleted { terminal: true; garbage_collection { after: P30D; mode: tombstone; } } initial Creating; terminal [Deleted]; Creating -> Deleted; } } }
SSPEC

cat > "$GC_EXPECTED" <<'SSPEC'
system Demo {
  entity Order {
    key order_id
    fields {
      order_id string
      created_at timestamp
      updated_at timestamp
      status string
    }
    state_machine {
      state Creating
      state Deleted {
        terminal: true
        garbage_collection {
          after: P30D
          mode: tombstone
        }
      }
      initial Creating
      terminal Deleted
      Creating -> Deleted
    }
  }
}
SSPEC

"$CLI" fmt "$GC_UNFORMATTED" > "$GC_FORMATTED"
diff -u "$GC_EXPECTED" "$GC_FORMATTED"

cat > "$INCLUDE_UNFORMATTED" <<'SSPEC'
statespec 0.1; include "./workflow-launch-control.sspec"; system Demo { }
SSPEC

cat > "$INCLUDE_EXPECTED" <<'SSPEC'
statespec 0.1;
include "./workflow-launch-control.sspec";
system Demo {
}
SSPEC

"$CLI" fmt "$INCLUDE_UNFORMATTED" > "$INCLUDE_FORMATTED"
diff -u "$INCLUDE_EXPECTED" "$INCLUDE_FORMATTED"

cat > "$API_SERVER_UNFORMATTED" <<'SSPEC'
system Demo { api StartOrderProcessing { method POST; path "/v1/orders/start"; } api_server OrderApi { serves StartOrderProcessing; concurrency 16; } }
SSPEC

cat > "$API_SERVER_EXPECTED" <<'SSPEC'
system Demo {
  api StartOrderProcessing {
    method POST
    path "/v1/orders/start"
  }

  api_server OrderApi {
    serves StartOrderProcessing
    concurrency 16
  }
}
SSPEC

"$CLI" fmt "$API_SERVER_UNFORMATTED" > "$API_SERVER_FORMATTED"
diff -u "$API_SERVER_EXPECTED" "$API_SERVER_FORMATTED"

cat > "$AST_UNFORMATTED" <<'SSPEC'
system Demo { entity Account { api { delete DeleteAccount; list Accounts { by [tenant_id]; path "/v1/tenants/{tenant_id}/accounts"; } resource "/v1/tenants/{tenant_id}/accounts/{account_id}"; create CreateAccount { fields [display_name]; } update_status UpdateAccountStatus; get GetAccount; } indexes { index by_tenant on tenant_id; } invariants { hasName: display_name != ""; } children { projects: Project by account_id; } relations { parent tenant_id: ref<Tenant> { unique_within_parent: [account_id]; on_parent_delete: block; kind: composition; } } state_machine { Active -> Deleted; initial Active; state Deleted { garbage_collection { mode: tombstone; after: P30D; } terminal: true; } state Active; terminal [Deleted]; } fields { created_at timestamp; updated_at timestamp; status string; tenant_id string; account_id string; display_name string; } ownership { lifecycle: authoritative; system_of_record: self; authority: system; } key tenant_id, account_id; } workflow AccountWorkflow { step create_account { max_retries 2; require account.status == Active; expected_execution_time PT5S; complete; } child_workflow tasks { failure when Task.status == Failed; success when Task.status == Active; create { task_id: task_id; account_id: account.account_id; tenant_id: account.tenant_id; } desired_count account.desired_task_count; parent_ref account_id = account.account_id; child_id task_id uuid; child_workflow TaskLifecycle; child_entity Task; } on AccountCreated; start create_account; expected_execution_time PT30S; singleton false; version 1; } }
SSPEC

cat > "$AST_EXPECTED" <<'SSPEC'
system Demo {
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
      account_id string
      display_name string
    }
    state_machine {
      state Deleted {
        terminal: true
        garbage_collection {
          after: P30D
          mode: tombstone
        }
      }
      state Active
      initial Active
      terminal Deleted
      Active -> Deleted
    }
    relations {
      parent tenant_id: ref<Tenant> {
        kind: composition
        on_parent_delete: block
        unique_within_parent: [account_id]
      }
    }
    children {
      projects: Project by account_id
    }
    invariants {
      hasName: display_name != ""
    }
    indexes {
      index by_tenant on tenant_id
    }
    api {
      resource "/v1/tenants/{tenant_id}/accounts/{account_id}"
      create CreateAccount {
        fields [display_name]
      }
      get GetAccount
      list Accounts {
        path "/v1/tenants/{tenant_id}/accounts"
        by [tenant_id]
      }
      update_status UpdateAccountStatus
      delete DeleteAccount
    }
  }

  workflow AccountWorkflow {
    version 1
    singleton false
    expected_execution_time PT30S
    start create_account
    on AccountCreated
    child_workflow tasks {
      child_entity Task
      child_workflow TaskLifecycle
      child_id task_id uuid
      parent_ref account_id = account.account_id
      desired_count account.desired_task_count
      create {
        task_id: task_id
        account_id: account.account_id
        tenant_id: account.tenant_id
      }
      success when Task.status == Active
      failure when Task.status == Failed
    }
    step create_account {
      expected_execution_time PT5S
      max_retries 2
      require account.status == Active;
      complete;
    }
  }
}
SSPEC

"$CLI" fmt "$AST_UNFORMATTED" > "$AST_FORMATTED"
diff -u "$AST_EXPECTED" "$AST_FORMATTED"

"$CLI" fmt --check "$PARITY_FIXTURE"

echo "formatter CLI tests passed"
