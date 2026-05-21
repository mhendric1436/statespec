#pragma once

#include "statespec/source.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

inline constexpr const char* DefaultTenantIdFieldName = "tenant_id";
inline constexpr const char* DefaultSystemTenantIdConfigKey = "STATESPEC_SYSTEM_TENANT_ID";

struct IncludeDecl
{
    std::string path;
    SourceRange range;
};

struct FieldDecl
{
    std::string name;
    std::string type;
    SourceRange range;
};

struct TenantScopeDecl
{
    std::string field_name = DefaultTenantIdFieldName;
    SourceRange range;
};

enum class SystemTenantSource
{
    Configured,
};

struct SystemTenantDecl
{
    SystemTenantSource source = SystemTenantSource::Configured;
    std::string config_key = DefaultSystemTenantIdConfigKey;
    SourceRange range;
};

struct BlockMemberOrder
{
    std::string kind;
    SourceRange range;
};

} // namespace statespec
