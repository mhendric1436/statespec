#pragma once

#include "statespec/ir_core.hpp"

namespace statespec
{

struct IrExternalSystemProperty
{
    std::string name;
    std::string value;
};

struct IrExternalSystemMetadataMapping
{
    std::string source;
    std::string target;
};

struct IrExternalSystemMetadata
{
    std::string entity;
    std::optional<std::string> tenant_field;
    std::string profile_field;
    std::vector<std::string> key_fields;
    std::vector<std::string> required_fields;
    std::vector<IrExternalSystemMetadataMapping> mappings;
};

struct IrExternalSystem
{
    std::string name;
    std::vector<IrExternalSystemProperty> properties;
    std::optional<IrExternalSystemMetadata> metadata;
};

} // namespace statespec
