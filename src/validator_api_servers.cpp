#include "validator_runtime.hpp"

#include "validator_helpers.hpp"

namespace statespec::validator_detail
{

void validate_api_servers(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& api_server : system.api_servers)
    {
        if (api_server.concurrency.has_value() && !is_positive_integer(*api_server.concurrency))
        {
            positive_integer_error(
                diagnostics, api_server.range, "api_server '" + api_server.name + "'", "concurrency"
            );
        }

        for (const auto& served_api : api_server.serves)
        {
            validate_symbol_reference(
                symbols, api_server.range, "api_server served API", served_api, {SymbolKind::Api},
                diagnostics
            );
        }
    }
}
} // namespace statespec::validator_detail
