#include "statespec/lexer.hpp"

namespace statespec {

Lexer::Lexer(const SourceFile& source)
    : source_(source)
{
}

std::vector<Token> Lexer::lex(DiagnosticBag& /*diagnostics*/)
{
    SourceLocation location{};
    location.offset = source_.text().size();

    return std::vector<Token>{Token{TokenKind::EndOfFile, "", SourceRange{location, location}}};
}

} // namespace statespec
