#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace statespec
{

enum class BindingLanguage
{
    Cpp,
    Go,
    Java,
    Rust,
};

class BindingLanguageError : public std::invalid_argument
{
  public:
    explicit BindingLanguageError(const std::string& message);
};

BindingLanguage parse_binding_language(std::string_view value);

std::string to_string(BindingLanguage language);

std::vector<std::string> supported_binding_languages();

std::string supported_binding_languages_text();

} // namespace statespec
