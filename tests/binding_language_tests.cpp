#include "statespec/binding_language.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace
{

void require(
    bool condition,
    const std::string& message
)
{
    if (!condition)
    {
        std::cerr << "binding_language_tests: " << message << '\n';
        std::exit(1);
    }
}

void require_equal(
    statespec::BindingLanguage actual,
    statespec::BindingLanguage expected,
    const std::string& message
)
{
    require(actual == expected, message);
}

void require_string_equal(
    const std::string& actual,
    const std::string& expected,
    const std::string& message
)
{
    require(actual == expected, message + ": expected '" + expected + "', got '" + actual + "'");
}

void test_parse_canonical_names()
{
    require_equal(
        statespec::parse_binding_language("cpp"), statespec::BindingLanguage::Cpp,
        "cpp should parse to Cpp"
    );
    require_equal(
        statespec::parse_binding_language("go"), statespec::BindingLanguage::Go,
        "go should parse to Go"
    );
    require_equal(
        statespec::parse_binding_language("java"), statespec::BindingLanguage::Java,
        "java should parse to Java"
    );
    require_equal(
        statespec::parse_binding_language("rust"), statespec::BindingLanguage::Rust,
        "rust should parse to Rust"
    );
}

void test_parse_aliases()
{
    require_equal(
        statespec::parse_binding_language("c++"), statespec::BindingLanguage::Cpp,
        "c++ should parse to Cpp"
    );
    require_equal(
        statespec::parse_binding_language("cplusplus"), statespec::BindingLanguage::Cpp,
        "cplusplus should parse to Cpp"
    );
    require_equal(
        statespec::parse_binding_language("golang"), statespec::BindingLanguage::Go,
        "golang should parse to Go"
    );
    require_equal(
        statespec::parse_binding_language("rs"), statespec::BindingLanguage::Rust,
        "rs should parse to Rust"
    );
}

void test_parse_is_case_and_separator_tolerant()
{
    require_equal(
        statespec::parse_binding_language(" Cpp "), statespec::BindingLanguage::Cpp,
        "Cpp with whitespace should parse to Cpp"
    );
    require_equal(
        statespec::parse_binding_language("GO"), statespec::BindingLanguage::Go,
        "GO should parse to Go"
    );
    require_equal(
        statespec::parse_binding_language("JaVa"), statespec::BindingLanguage::Java,
        "JaVa should parse to Java"
    );
    require_equal(
        statespec::parse_binding_language("r-u_s-t"), statespec::BindingLanguage::Rust,
        "r-u_s-t should parse to Rust"
    );
}

void test_to_string()
{
    require_string_equal(statespec::to_string(statespec::BindingLanguage::Cpp), "cpp", "Cpp text");
    require_string_equal(statespec::to_string(statespec::BindingLanguage::Go), "go", "Go text");
    require_string_equal(statespec::to_string(statespec::BindingLanguage::Java), "java", "Java text");
    require_string_equal(statespec::to_string(statespec::BindingLanguage::Rust), "rust", "Rust text");
}

void test_supported_languages()
{
    const auto languages = statespec::supported_binding_languages();
    require(languages.size() == 4, "expected four supported binding languages");
    require_string_equal(languages[0], "cpp", "supported language 0");
    require_string_equal(languages[1], "go", "supported language 1");
    require_string_equal(languages[2], "java", "supported language 2");
    require_string_equal(languages[3], "rust", "supported language 3");
    require_string_equal(
        statespec::supported_binding_languages_text(), "cpp|go|java|rust",
        "supported language text"
    );
}

void test_invalid_language_throws()
{
    bool threw = false;
    try
    {
        (void)statespec::parse_binding_language("python");
    }
    catch (const statespec::BindingLanguageError& ex)
    {
        threw = true;
        const std::string message = ex.what();
        require(
            message.find("python") != std::string::npos,
            "invalid language error should include rejected value"
        );
        require(
            message.find("cpp|go|java|rust") != std::string::npos,
            "invalid language error should include supported languages"
        );
    }
    catch (...)
    {
        require(false, "invalid language should throw BindingLanguageError");
    }

    require(threw, "invalid language should throw");
}

} // namespace

int main()
{
    test_parse_canonical_names();
    test_parse_aliases();
    test_parse_is_case_and_separator_tolerant();
    test_to_string();
    test_supported_languages();
    test_invalid_language_throws();

    return 0;
}
