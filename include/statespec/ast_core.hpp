#pragma once

#include "statespec/language_constants.hpp"
#include "statespec/source.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

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
    std::string field_name = std::string{DefaultTenantIdFieldName};
    SourceRange range;
};

enum class SystemTenantSource
{
    Configured,
};

struct SystemTenantDecl
{
    SystemTenantSource source = SystemTenantSource::Configured;
    std::string config_key = std::string{DefaultSystemTenantIdConfigKey};
    SourceRange range;
};

struct BlockMemberOrder
{
    std::string kind;
    SourceRange range;
};

} // namespace statespec
