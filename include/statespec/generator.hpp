#pragma once

#include "statespec/ast.hpp"
#include <string>
#include <vector>

namespace statespec
{

struct GeneratedFile
{
    std::string path;
    std::string content;
};

struct GenerationResult
{
    std::vector<GeneratedFile> files;
};

} // namespace statespec
