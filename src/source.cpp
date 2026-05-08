#include "statespec/source.hpp"

#include <utility>

namespace statespec
{

SourceFile::SourceFile(
    std::string path,
    std::string text
)
    : path_(std::move(path)),
      text_(std::move(text))
{
}

const std::string& SourceFile::path() const noexcept
{
    return path_;
}

const std::string& SourceFile::text() const noexcept
{
    return text_;
}

} // namespace statespec
