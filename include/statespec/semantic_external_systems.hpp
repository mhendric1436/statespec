#pragma once

#include "statespec/semantic_core.hpp"

namespace statespec
{

struct SemanticExternalSystemProperty
{
    std::string name;
    std::string value;
};

struct SemanticExternalSystemMetadataMapping
{
    std::string source;
    std::string target;
};

struct SemanticExternalSystemMetadata
{
    std::string entity;
    std::optional<std::string> tenant_field;
    std::string profile_field;
    std::vector<std::string> key_fields;
    std::vector<std::string> required_fields;
    std::vector<SemanticExternalSystemMetadataMapping> mappings;
};

struct SemanticExternalSystem
{
    std::string name;
    std::vector<SemanticExternalSystemProperty> properties;
    std::optional<SemanticExternalSystemMetadata> metadata;
};

} // namespace statespec
