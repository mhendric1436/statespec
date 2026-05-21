#pragma once

#include "statespec/ast_core.hpp"

namespace statespec
{

struct ExternalSystemPropertyDecl
{
    std::string name;
    std::string value;
    std::optional<std::string> value_kind;
    SourceRange range;
};

struct ExternalSystemMetadataMappingDecl
{
    std::string source;
    std::string target;
    SourceRange range;
};

struct ExternalSystemMetadataDecl
{
    std::optional<std::string> entity;
    std::optional<std::string> profile_field;
    std::vector<std::string> required_fields;
    std::vector<ExternalSystemMetadataMappingDecl> mappings;
    SourceRange range;
};

struct ExternalSystemDecl
{
    std::string name;
    std::vector<ExternalSystemPropertyDecl> properties;
    std::optional<ExternalSystemMetadataDecl> metadata;
    SourceRange range;
};

} // namespace statespec
