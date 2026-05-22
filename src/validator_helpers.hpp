#pragma once

#include "statespec/ast.hpp"
#include "statespec/language_constants.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace statespec::validator_detail
{

inline bool contains(
    const std::unordered_set<std::string>& values,
    const std::string& value
)
{
    return values.find(value) != values.end();
}

inline bool is_builtin_type(const std::string& type)
{
    static const std::unordered_set<std::string> builtin_types{
        "string",    "bool",     "int",  "int32", "int64", "long", "double",   "decimal", "json",
        "timestamp", "duration", "uuid", "list",  "set",   "map",  "optional", "ref",
    };

    if (!type.empty() && type.back() == '?')
    {
        return builtin_types.find(type.substr(0, type.size() - 1)) != builtin_types.end();
    }

    return builtin_types.find(type) != builtin_types.end();
}

inline std::string base_type_name(const std::string& type)
{
    const auto generic = type.find('<');
    if (generic != std::string::npos)
    {
        return type.substr(0, generic);
    }

    if (type.size() >= 2 && type.substr(type.size() - 2) == "[]")
    {
        return type.substr(0, type.size() - 2);
    }

    if (!type.empty() && type.back() == '?')
    {
        return type.substr(0, type.size() - 1);
    }

    return type;
}

inline bool is_supported_feature_flag_type(const std::string& type)
{
    static const std::unordered_set<std::string> feature_flag_types{
        "bool",
        "string",
        "int",
        "decimal",
    };
    return feature_flag_types.find(type) != feature_flag_types.end();
}

inline bool is_supported_log_level(const std::string& level)
{
    static const std::unordered_set<std::string> log_levels{
        "debug",
        "info",
        "warn",
        "error",
    };
    return log_levels.find(level) != log_levels.end();
}

inline bool is_supported_metric_kind(const std::string& kind)
{
    static const std::unordered_set<std::string> metric_kinds{
        "counter",
        "gauge",
        "histogram",
    };
    return metric_kinds.find(kind) != metric_kinds.end();
}

inline bool is_supported_metric_label_type(const std::string& type)
{
    const auto base_type = base_type_name(type);
    return base_type == "string" || base_type == "bool" || base_type == "int";
}

inline bool is_pascal_case_name(const std::string& name)
{
    if (name.empty() || std::isupper(static_cast<unsigned char>(name.front())) == 0)
    {
        return false;
    }

    for (const auto ch : name)
    {
        const auto value = static_cast<unsigned char>(ch);
        if (std::isalnum(value) == 0)
        {
            return false;
        }
    }

    return true;
}

inline bool is_qualified_pascal_case_name(const std::string& name)
{
    if (name.empty())
    {
        return false;
    }

    std::size_t start = 0;
    while (start < name.size())
    {
        const auto end = name.find('.', start);
        const auto segment =
            name.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!is_pascal_case_name(segment))
        {
            return false;
        }
        if (end == std::string::npos)
        {
            return true;
        }
        start = end + 1;
    }

    return false;
}

inline bool is_positive_integer(int value)
{
    return value > 0;
}

inline bool is_non_negative_integer(int value)
{
    return value >= 0;
}

inline bool is_duration_literal(const std::string& value)
{
    if (value.size() < 3 || value.front() != 'P')
    {
        return false;
    }

    bool has_digit = false;
    bool has_unit = false;
    bool in_time = false;

    for (std::size_t i = 1; i < value.size(); ++i)
    {
        const char ch = value[i];
        if (std::isdigit(static_cast<unsigned char>(ch)) != 0)
        {
            has_digit = true;
            continue;
        }
        if (ch == 'T')
        {
            if (in_time)
            {
                return false;
            }
            in_time = true;
            continue;
        }
        if (ch == 'D' || ch == 'H' || ch == 'M' || ch == 'S')
        {
            has_unit = true;
            continue;
        }
        return false;
    }

    return has_digit && has_unit;
}

inline bool is_garbage_collection_mode(const std::string& value)
{
    return value == GarbageCollectionModeDelete || value == GarbageCollectionModeTombstone ||
           value == GarbageCollectionModeArchive;
}

inline bool is_ownership_authority(const std::string& value)
{
    return value == OwnershipAuthoritySystem || value == OwnershipAuthorityExternal;
}

inline bool is_lifecycle_mode(const std::string& value)
{
    return value == OwnershipLifecycleAuthoritative || value == OwnershipLifecycleManaged ||
           value == OwnershipLifecycleObserved || value == OwnershipLifecycleProjected;
}

inline bool is_relation_kind(const std::string& value)
{
    return value == "composition" || value == "aggregation" || value == "reference";
}

inline bool is_parent_delete_behavior(const std::string& value)
{
    return value == "cascade" || value == "block" || value == "detach" || value == "fail";
}

inline bool is_integer_text(const std::string& value)
{
    if (value.empty())
    {
        return false;
    }

    return std::all_of(
        value.begin(), value.end(),
        [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)) != 0; }
    );
}

inline bool feature_flag_default_matches_type(const FeatureFlagDecl& flag)
{
    if (!flag.type.has_value() || !flag.default_value.has_value() ||
        !flag.default_value_kind.has_value())
    {
        return true;
    }

    if (*flag.type == "bool")
    {
        return *flag.default_value_kind == "BooleanLiteral";
    }
    if (*flag.type == "string")
    {
        return *flag.default_value_kind == "StringLiteral";
    }
    if (*flag.type == "int")
    {
        return *flag.default_value_kind == "IntegerLiteral" && is_integer_text(*flag.default_value);
    }
    if (*flag.type == "decimal")
    {
        return *flag.default_value_kind == "DecimalLiteral" ||
               *flag.default_value_kind == "IntegerLiteral";
    }

    return true;
}

inline std::optional<std::string> entity_scope_target(const std::string& scope)
{
    constexpr std::string_view prefix{"entity "};
    if (scope.rfind(std::string{prefix}, 0) != 0)
    {
        return std::nullopt;
    }
    return scope.substr(prefix.size());
}

inline std::string queue_message_name(
    const QueueDecl& queue,
    const MessageDecl& message
)
{
    return queue.name + "." + message.name;
}

inline std::string workflow_step_name(
    const WorkflowDecl& workflow,
    const WorkflowStepDecl& step
)
{
    return workflow.name + "." + step.name;
}

inline std::unordered_set<std::string> field_names(const std::vector<FieldDecl>& fields)
{
    std::unordered_set<std::string> names;
    for (const auto& field : fields)
    {
        names.insert(field.name);
    }
    return names;
}

inline std::string relation_target_name(std::string target)
{
    if (!target.empty() && target.back() == '?')
    {
        target.pop_back();
    }
    constexpr std::string_view optional_prefix{"optional "};
    if (target.rfind(std::string{optional_prefix}, 0) == 0)
    {
        target = target.substr(optional_prefix.size());
    }
    constexpr std::string_view ref_prefix{"ref<"};
    if (target.rfind(std::string{ref_prefix}, 0) == 0 && target.back() == '>')
    {
        return target.substr(ref_prefix.size(), target.size() - ref_prefix.size() - 1);
    }
    return target;
}

} // namespace statespec::validator_detail
