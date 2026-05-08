#pragma once

#include <cstddef>
#include <string>

namespace statespec
{

struct SourceLocation
{
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct SourceRange
{
    SourceLocation begin;
    SourceLocation end;
};

class SourceFile
{
  public:
    SourceFile(
        std::string path,
        std::string text
    );

    const std::string& path() const noexcept;
    const std::string& text() const noexcept;

  private:
    std::string path_;
    std::string text_;
};

} // namespace statespec
