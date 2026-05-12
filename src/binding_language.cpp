#include "statespec/binding_language.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace statespec
{

namespace
{

std::string normalize_language_name(std::string_view value)
{
    std::string normalized;
    normalized.reserve(value.size());

    for (const auto raw_ch : value)
    {
        const auto ch = static_cast<unsigned char>(raw_ch);
        if (!std::isspace(ch) && raw_ch != '-' && raw_ch != '_')
        {
            normalized.push_back(static_cast<char>(std::tolower(ch)));
        }
    }

    return normalized;
}

} // namespace

BindingLanguageError::BindingLanguageError(const std::string& message)
    : std::invalid_argument(message)
{
}

BindingLanguage parse_binding_language(std::string_view value)
{
    const auto normalized = normalize_language_name(value);

    if (normalized == "cpp" || normalized == "c++" || normalized == "cplusplus")
    {
        return BindingLanguage::Cpp;
    }
    if (normalized == "go" || normalized == "golang")
    {
        return BindingLanguage::Go;
    }
    if (normalized == "java")
    {
        return BindingLanguage::Java;
    }
    if (normalized == "rust" || normalized == "rs")
    {
        return BindingLanguage::Rust;
    }

    throw BindingLanguageError(
        "unsupported binding language '" + std::string(value) + "'; supported languages: " +
        supported_binding_languages_text()
    );
}

std::string to_string(BindingLanguage language)
{
    switch (language)
    {
    case BindingLanguage::Cpp:
        return "cpp";
    case BindingLanguage::Go:
        return "go";
    case BindingLanguage::Java:
        return "java";
    case BindingLanguage::Rust:
        return "rust";
    }

    throw BindingLanguageError("unsupported binding language enum value");
}

std::vector<std::string> supported_binding_languages()
{
    return {"cpp", "go", "java", "rust"};
}

std::string supported_binding_languages_text()
{
    const auto languages = supported_binding_languages();
    std::ostringstream out;
    for (std::size_t i = 0; i < languages.size(); ++i)
    {
        if (i > 0)
        {
            out << '|';
        }
        out << languages[i];
    }
    return out.str();
}

} // namespace statespec
