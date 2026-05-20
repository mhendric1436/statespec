#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/template_renderer.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace
{

void renderer_replaces_placeholders()
{
    const statespec::TemplateRenderer renderer;
    const auto output = renderer.render(
        "namespace {{ namespace_name }} {\n{{ body }}\n}\n", statespec::TemplateRenderer::Values{
                                                                 {"body", "int value = 42;"},
                                                                 {"namespace_name", "example"},
                                                             }
    );

    statespec::test::require(
        output == "namespace example {\nint value = 42;\n}\n",
        "template renderer should replace named placeholders"
    );
}

void renderer_allows_repeated_placeholders()
{
    const statespec::TemplateRenderer renderer;
    const auto output = renderer.render(
        "{{ name }}::{{ name }}", statespec::TemplateRenderer::Values{{"name", "Order"}}
    );

    statespec::test::require(
        output == "Order::Order", "template renderer should replace repeated placeholders"
    );
}

void renderer_rejects_missing_values()
{
    const statespec::TemplateRenderer renderer;

    REQUIRE_THROWS_WITH(
        renderer.render("{{ missing }}", statespec::TemplateRenderer::Values{}),
        "template placeholder 'missing' has no render value"
    );
}

void renderer_rejects_unused_values()
{
    const statespec::TemplateRenderer renderer;

    REQUIRE_THROWS_WITH(
        renderer.render("static text", statespec::TemplateRenderer::Values{{"unused", "value"}}),
        "render value 'unused' was not used by template"
    );
}

void renderer_rejects_unclosed_placeholders()
{
    const statespec::TemplateRenderer renderer;

    REQUIRE_THROWS_WITH(
        renderer.render("{{ name", statespec::TemplateRenderer::Values{{"name", "Order"}}),
        "template placeholder opened without closing delimiter"
    );
}

void renderer_rejects_empty_placeholders()
{
    const statespec::TemplateRenderer renderer;

    REQUIRE_THROWS_WITH(
        renderer.render("{{ }}", statespec::TemplateRenderer::Values{}),
        "template placeholder name must not be empty"
    );
}

void renderer_loads_template_files()
{
    const auto path =
        std::filesystem::temp_directory_path() / "statespec-template-renderer-test.template";
    {
        std::ofstream output(path);
        output << "hello {{ target }}\n";
    }

    const statespec::TemplateRenderer renderer;
    const auto rendered =
        renderer.render_file(path, statespec::TemplateRenderer::Values{{"target", "templates"}});

    std::filesystem::remove(path);

    statespec::test::require(
        rendered == "hello templates\n", "template renderer should load and render files"
    );
}

void renderer_rejects_missing_template_files()
{
    const statespec::TemplateRenderer renderer;
    const auto path =
        std::filesystem::temp_directory_path() / "statespec-template-renderer-missing.template";
    std::filesystem::remove(path);

    REQUIRE_THROWS_AS(
        renderer.render_file(path, statespec::TemplateRenderer::Values{}), std::runtime_error
    );
}

} // namespace

TEST_CASE("template renderer replaces placeholders")
{
    renderer_replaces_placeholders();
}

TEST_CASE("template renderer allows repeated placeholders")
{
    renderer_allows_repeated_placeholders();
}

TEST_CASE("template renderer rejects missing values")
{
    renderer_rejects_missing_values();
}

TEST_CASE("template renderer rejects unused values")
{
    renderer_rejects_unused_values();
}

TEST_CASE("template renderer rejects unclosed placeholders")
{
    renderer_rejects_unclosed_placeholders();
}

TEST_CASE("template renderer rejects empty placeholders")
{
    renderer_rejects_empty_placeholders();
}

TEST_CASE("template renderer loads template files")
{
    renderer_loads_template_files();
}

TEST_CASE("template renderer rejects missing template files")
{
    renderer_rejects_missing_template_files();
}
