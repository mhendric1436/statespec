#include "binding_test_support.hpp"

namespace
{

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
    require_string_equal(
        statespec::to_string(statespec::BindingLanguage::Java), "java", "Java text"
    );
    require_string_equal(
        statespec::to_string(statespec::BindingLanguage::Rust), "rust", "Rust text"
    );
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
        statespec::supported_binding_languages_text(), "cpp|go|java|rust", "supported language text"
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

void test_field_descriptor_type_classification()
{
    require_field_type("string", statespec::FieldDescriptorType::String, "string");
    require_field_type("bool", statespec::FieldDescriptorType::Bool, "bool");
    require_field_type("int", statespec::FieldDescriptorType::Int, "int");
    require_field_type("int32", statespec::FieldDescriptorType::Int32, "int32");
    require_field_type("int64", statespec::FieldDescriptorType::Int64, "int64");
    require_field_type("long", statespec::FieldDescriptorType::Long, "long");
    require_field_type("double", statespec::FieldDescriptorType::Double, "double");
    require_field_type("decimal", statespec::FieldDescriptorType::Decimal, "decimal");
    require_field_type("json", statespec::FieldDescriptorType::Json, "json");
    require_field_type("timestamp", statespec::FieldDescriptorType::Timestamp, "timestamp");
    require_field_type("duration", statespec::FieldDescriptorType::Duration, "duration");
    require_field_type("uuid", statespec::FieldDescriptorType::Uuid, "uuid");
    require_field_type("OrderStatus", statespec::FieldDescriptorType::Named, "named");
    require_field_type("list<string>", statespec::FieldDescriptorType::List, "list");
    require_field_type("string[]", statespec::FieldDescriptorType::List, "list");
    require_field_type("set<uuid>", statespec::FieldDescriptorType::Set, "set");
    require_field_type("map<string,json>", statespec::FieldDescriptorType::Map, "map");
    require_field_type("optional<string>", statespec::FieldDescriptorType::Optional, "optional");
    require_field_type("OrderStatus?", statespec::FieldDescriptorType::Optional, "optional");
    require_field_type("ref<Account>", statespec::FieldDescriptorType::Reference, "reference");
    require_field_type("  uuid  ", statespec::FieldDescriptorType::Uuid, "uuid");
}

} // namespace

TEST_CASE("binding language parses canonical names")
{
    test_parse_canonical_names();
}

TEST_CASE("binding language parses aliases")
{
    test_parse_aliases();
}

TEST_CASE("binding language parsing is case and separator tolerant")
{
    test_parse_is_case_and_separator_tolerant();
}

TEST_CASE("binding language converts to strings")
{
    test_to_string();
}

TEST_CASE("binding language lists supported languages")
{
    test_supported_languages();
}

TEST_CASE("binding language rejects unsupported languages")
{
    test_invalid_language_throws();
}

TEST_CASE("field descriptor type classification normalizes grammar types")
{
    test_field_descriptor_type_classification();
}
