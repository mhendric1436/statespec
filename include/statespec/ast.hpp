#pragma once

#include "statespec/source.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

struct ImportDecl
{
    std::string name;
    std::optional<std::string> alias;
    SourceRange range;
};

struct SystemDecl
{
    std::string name;
    SourceRange range;
};

struct Spec
{
    std::optional<std::string> version;
    std::vector<ImportDecl> imports;
    std::optional<SystemDecl> system;
};

} // namespace statespec
