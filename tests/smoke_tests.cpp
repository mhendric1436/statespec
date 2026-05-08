#include "statespec/diagnostic.hpp"
#include "statespec/lexer.hpp"
#include "statespec/source.hpp"
#include "statespec/symbol_table.hpp"
#include "statespec/token.hpp"
#include "statespec/validator.hpp"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "test failed: " << message << '\n';
        std::exit(1);
    }
}

void diagnostic_bag_records_errors()
{
    statespec::DiagnosticBag diagnostics;
    require(!diagnostics.has_errors(), "new DiagnosticBag should not have errors");
    diagnostics.error(statespec::SourceRange{}, "TEST", "example");
    require(diagnostics.has_errors(), "DiagnosticBag should report errors");
    require(diagnostics.all().size() == 1, "DiagnosticBag should store diagnostics");
}

void symbol_table_rejects_duplicates()
{
    statespec::SymbolTable table;
    require(table.insert(statespec::Symbol{statespec::SymbolKind::Entity, "Order", {}}), "first symbol insert should succeed");
    require(!table.insert(statespec::Symbol{statespec::SymbolKind::Entity, "Order", {}}), "duplicate symbol insert should fail");
    require(table.find("Order").has_value(), "inserted symbol should be findable");
}

void lexer_emits_eof_for_empty_source()
{
    statespec::SourceFile source{"empty.sspec", ""};
    statespec::DiagnosticBag diagnostics;
    statespec::Lexer lexer{source};
    auto tokens = lexer.lex(diagnostics);
    require(tokens.size() == 1, "empty source should produce one EOF token");
    require(tokens[0].kind == statespec::TokenKind::EndOfFile, "empty source token should be EOF");
}

void validator_rejects_missing_system()
{
    statespec::Spec spec;
    statespec::DiagnosticBag diagnostics;
    statespec::Validator validator;
    validator.validate(spec, diagnostics);
    require(diagnostics.has_errors(), "validator should reject spec without system");
}

} // namespace

int main()
{
    diagnostic_bag_records_errors();
    symbol_table_rejects_duplicates();
    lexer_emits_eof_for_empty_source();
    validator_rejects_missing_system();

    std::cout << "smoke tests passed\n";
    return 0;
}
