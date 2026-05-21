#include "validator_runtime.hpp"

#include "validator_helpers.hpp"

#include <cctype>
#include <cstdint>
#include <optional>
#include <string>

namespace statespec::validator_detail
{

namespace
{

std::optional<std::int64_t> duration_seconds(const std::string& text)
{
    if (!is_duration_literal(text))
    {
        return std::nullopt;
    }

    std::int64_t total = 0;
    std::int64_t current = 0;
    bool in_time = false;
    for (std::size_t i = 1; i < text.size(); ++i)
    {
        const auto ch = text[i];
        if (std::isdigit(static_cast<unsigned char>(ch)) != 0)
        {
            current = current * 10 + static_cast<std::int64_t>(ch - '0');
            continue;
        }
        if (ch == 'T')
        {
            in_time = true;
            continue;
        }
        if (ch == 'D')
        {
            total += current * 24 * 60 * 60;
        }
        else if (in_time && ch == 'H')
        {
            total += current * 60 * 60;
        }
        else if (in_time && ch == 'M')
        {
            total += current * 60;
        }
        else if (in_time && ch == 'S')
        {
            total += current;
        }
        else
        {
            return std::nullopt;
        }
        current = 0;
    }

    return total;
}
} // namespace

void validate_leases(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    for (const auto& lease : system.leases)
    {
        if (!lease.resource.has_value())
        {
            required_error(diagnostics, lease.range, "lease '" + lease.name + "'", "resource");
        }
        if (!lease.ttl.has_value())
        {
            required_error(diagnostics, lease.range, "lease '" + lease.name + "'", "ttl");
        }
        else if (!is_duration_literal(*lease.ttl))
        {
            duration_error(diagnostics, lease.range, "lease '" + lease.name + "'", "ttl");
        }
        if (!lease.renew_every.has_value())
        {
            required_error(diagnostics, lease.range, "lease '" + lease.name + "'", "renew_every");
        }
        else if (!is_duration_literal(*lease.renew_every))
        {
            duration_error(diagnostics, lease.range, "lease '" + lease.name + "'", "renew_every");
        }
        if (!lease.holder.has_value())
        {
            required_error(diagnostics, lease.range, "lease '" + lease.name + "'", "holder");
        }
        if (!lease.fencing_token.has_value())
        {
            required_error(diagnostics, lease.range, "lease '" + lease.name + "'", "fencing_token");
        }
        if (!lease.max_ttl.has_value())
        {
            required_error(diagnostics, lease.range, "lease '" + lease.name + "'", "max_ttl");
        }
        else if (!is_duration_literal(*lease.max_ttl))
        {
            duration_error(diagnostics, lease.range, "lease '" + lease.name + "'", "max_ttl");
        }
        if (lease.ttl.has_value() && lease.renew_every.has_value() &&
            is_duration_literal(*lease.ttl) && is_duration_literal(*lease.renew_every))
        {
            const auto ttl = duration_seconds(*lease.ttl);
            const auto renew_every = duration_seconds(*lease.renew_every);
            if (ttl.has_value() && renew_every.has_value() && *renew_every >= *ttl)
            {
                diagnostics.error(
                    lease.range, "SSPEC3501",
                    "lease '" + lease.name + "' renew_every must be less than ttl"
                );
            }
        }
        if (lease.ttl.has_value() && lease.max_ttl.has_value() && is_duration_literal(*lease.ttl) &&
            is_duration_literal(*lease.max_ttl))
        {
            const auto ttl = duration_seconds(*lease.ttl);
            const auto max_ttl = duration_seconds(*lease.max_ttl);
            if (ttl.has_value() && max_ttl.has_value() && *ttl > *max_ttl)
            {
                diagnostics.error(
                    lease.range, "SSPEC3502",
                    "lease '" + lease.name + "' ttl must be less than or equal to max_ttl"
                );
            }
        }
    }
}
} // namespace statespec::validator_detail
