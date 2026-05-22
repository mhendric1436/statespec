#include "validator_runtime.hpp"

#include "validator_helpers.hpp"

namespace statespec::validator_detail
{

void validate_workers(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& worker : system.workers)
    {
        if (worker.singleton.value_or(false) && !worker.lease.has_value())
        {
            diagnostics.error(
                worker.range, diagnostic_codes::OwnershipInvalidAuthority,
                "singleton worker '" + worker.name + "' must declare a lease"
            );
        }
        if (worker.concurrency.has_value() && !is_positive_integer(*worker.concurrency))
        {
            positive_integer_error(
                diagnostics, worker.range, "worker '" + worker.name + "'", "concurrency"
            );
        }

        if (worker.lease.has_value() && !symbols.find(*worker.lease).has_value())
        {
            unknown_reference_error(diagnostics, worker.range, "worker lease", *worker.lease);
        }
        if (worker.polls.has_value() && !symbols.find(*worker.polls).has_value())
        {
            unknown_reference_error(
                diagnostics, worker.range, "worker polls target", *worker.polls
            );
        }
        if (worker.executes.has_value() && !symbols.find(*worker.executes).has_value())
        {
            unknown_reference_error(
                diagnostics, worker.range, "worker executes target", *worker.executes
            );
        }
    }
}
} // namespace statespec::validator_detail
