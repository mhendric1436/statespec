#pragma once

#include "statespec/ast.hpp"
#include <string>
#include <vector>

namespace statespec
{

enum class GeneratedArtifactTier
{
    Common,
    Api,
    Worker,
};

struct GeneratedFile
{
    std::string path;
    std::string content;
    GeneratedArtifactTier tier = GeneratedArtifactTier::Common;
};

struct GenerationResult
{
    std::vector<GeneratedFile> files;
};

} // namespace statespec
