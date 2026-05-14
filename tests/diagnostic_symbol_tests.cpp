#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/symbol_table.hpp"

namespace
{

void diagnostic_bag_records_errors()
{
    statespec::DiagnosticBag diagnostics;
    statespec::test::require(!diagnostics.has_errors(), "new DiagnosticBag should not have errors");
    diagnostics.error(statespec::SourceRange{}, "TEST", "example");
    statespec::test::require(diagnostics.has_errors(), "DiagnosticBag should report errors");
    statespec::test::require(
        diagnostics.all().size() == 1, "DiagnosticBag should store diagnostics"
    );
}

void symbol_table_rejects_duplicates()
{
    statespec::SymbolTable table;
    statespec::test::require(
        table.insert(statespec::Symbol{statespec::SymbolKind::Entity, "Order", {}}),
        "first symbol insert should succeed"
    );
    statespec::test::require(
        !table.insert(statespec::Symbol{statespec::SymbolKind::Entity, "Order", {}}),
        "duplicate symbol insert should fail"
    );
    statespec::test::require(table.find("Order").has_value(), "inserted symbol should be findable");
}

} // namespace

TEST_CASE("diagnostic bag records errors")
{
    diagnostic_bag_records_errors();
}

TEST_CASE("symbol table rejects duplicates")
{
    symbol_table_rejects_duplicates();
}
