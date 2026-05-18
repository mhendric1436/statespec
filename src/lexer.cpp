#include "statespec/lexer.hpp"

#include <cctype>
#include <string_view>
#include <unordered_map>

namespace statespec
{

namespace
{

bool is_identifier_start(char ch)
{
    const auto value = static_cast<unsigned char>(ch);
    return std::isalpha(value) != 0 || ch == '_';
}

bool is_identifier_part(char ch)
{
    const auto value = static_cast<unsigned char>(ch);
    return std::isalnum(value) != 0 || ch == '_';
}

bool is_digit(char ch)
{
    return std::isdigit(static_cast<unsigned char>(ch)) != 0;
}

TokenKind keyword_kind(std::string_view text)
{
    static const std::unordered_map<std::string_view, TokenKind> keywords{
        {"statespec", TokenKind::KeywordStatespec},
        {"include", TokenKind::KeywordInclude},
        {"import", TokenKind::KeywordImport},
        {"as", TokenKind::KeywordAs},
        {"system", TokenKind::KeywordSystem},
        {"namespace", TokenKind::KeywordNamespace},
        {"value", TokenKind::KeywordValue},
        {"enum", TokenKind::KeywordEnum},
        {"shape", TokenKind::KeywordShape},
        {"external_system", TokenKind::KeywordExternalSystem},
        {"feature_flag", TokenKind::KeywordFeatureFlag},
        {"log", TokenKind::KeywordLog},
        {"metric", TokenKind::KeywordMetric},
        {"entity", TokenKind::KeywordEntity},
        {"key", TokenKind::KeywordKey},
        {"version", TokenKind::KeywordVersion},
        {"fields", TokenKind::KeywordFields},
        {"state_machine", TokenKind::KeywordStateMachine},
        {"state", TokenKind::KeywordState},
        {"initial", TokenKind::KeywordInitial},
        {"terminal", TokenKind::KeywordTerminal},
        {"ownership", TokenKind::KeywordOwnership},
        {"authority", TokenKind::KeywordAuthority},
        {"system_of_record", TokenKind::KeywordSystemOfRecord},
        {"lifecycle", TokenKind::KeywordLifecycle},
        {"control", TokenKind::KeywordControl},
        {"relations", TokenKind::KeywordRelations},
        {"parent", TokenKind::KeywordParent},
        {"ref", TokenKind::KeywordRef},
        {"optional", TokenKind::KeywordOptional},
        {"children", TokenKind::KeywordChildren},
        {"invariants", TokenKind::KeywordInvariants},
        {"indexes", TokenKind::KeywordIndexes},
        {"annotations", TokenKind::KeywordAnnotations},
        {"event", TokenKind::KeywordEvent},
        {"queue", TokenKind::KeywordQueue},
        {"message", TokenKind::KeywordMessage},
        {"payload", TokenKind::KeywordPayload},
        {"lease", TokenKind::KeywordLease},
        {"worker", TokenKind::KeywordWorker},
        {"api_server", TokenKind::KeywordApiServer},
        {"api", TokenKind::KeywordApi},
        {"behavior", TokenKind::KeywordBehavior},
        {"workflow", TokenKind::KeywordWorkflow},
        {"step", TokenKind::KeywordStep},
        {"child_set", TokenKind::KeywordChildSet},
        {"policy", TokenKind::KeywordPolicy},
        {"on", TokenKind::KeywordOn},
        {"when", TokenKind::KeywordWhen},
        {"where", TokenKind::KeywordWhere},
        {"require", TokenKind::KeywordRequire},
        {"set", TokenKind::KeywordSet},
        {"emit", TokenKind::KeywordEmit},
        {"emits", TokenKind::KeywordEmits},
        {"enqueue", TokenKind::KeywordEnqueue},
        {"acquire", TokenKind::KeywordAcquire},
        {"renew", TokenKind::KeywordRenew},
        {"release", TokenKind::KeywordRelease},
        {"start", TokenKind::KeywordStart},
        {"load", TokenKind::KeywordLoad},
        {"allocates", TokenKind::KeywordAllocates},
        {"returns", TokenKind::KeywordReturns},
        {"atomic", TokenKind::KeywordAtomic},
        {"for_each", TokenKind::KeywordForEach},
        {"in", TokenKind::KeywordIn},
        {"create", TokenKind::KeywordCreate},
        {"child", TokenKind::KeywordChild},
        {"observe", TokenKind::KeywordObserve},
        {"move", TokenKind::KeywordMove},
        {"from", TokenKind::KeywordFrom},
        {"to", TokenKind::KeywordTo},
        {"transition_to", TokenKind::KeywordTransitionTo},
        {"complete", TokenKind::KeywordComplete},
        {"fail", TokenKind::KeywordFail},
        {"reserve", TokenKind::KeywordReserve},
        {"materialize", TokenKind::KeywordMaterialize},
        {"reconcile", TokenKind::KeywordReconcile},
        {"allow", TokenKind::KeywordAllow},
        {"deny", TokenKind::KeywordDeny},
        {"quota", TokenKind::KeywordQuota},
        {"audit", TokenKind::KeywordAudit},
        {"tenant", TokenKind::KeywordTenant},
        {"scoped_by", TokenKind::KeywordScopedBy},
        {"method", TokenKind::KeywordMethod},
        {"path", TokenKind::KeywordPath},
        {"input", TokenKind::KeywordInput},
        {"output", TokenKind::KeywordOutput},
        {"error", TokenKind::KeywordError},
        {"authz", TokenKind::KeywordAuthz},
        {"starts", TokenKind::KeywordStarts},
        {"enqueues", TokenKind::KeywordEnqueues},
        {"serves", TokenKind::KeywordServes},
        {"polls", TokenKind::KeywordPolls},
        {"executes", TokenKind::KeywordExecutes},
        {"singleton", TokenKind::KeywordSingleton},
        {"concurrency", TokenKind::KeywordConcurrency},
    };

    if (text == "true" || text == "false")
    {
        return TokenKind::BooleanLiteral;
    }

    const auto it = keywords.find(text);
    if (it == keywords.end())
    {
        return TokenKind::Identifier;
    }
    return it->second;
}

bool looks_like_duration(std::string_view text)
{
    if (text.empty() || text.front() != 'P')
    {
        return false;
    }

    bool has_digit = false;
    bool has_unit = false;
    bool in_time = false;

    for (std::size_t i = 1; i < text.size(); ++i)
    {
        const char ch = text[i];
        if (is_digit(ch))
        {
            has_digit = true;
            continue;
        }
        if (ch == 'T')
        {
            if (in_time)
            {
                return false;
            }
            in_time = true;
            continue;
        }
        if (ch == 'D' || ch == 'H' || ch == 'M' || ch == 'S')
        {
            has_unit = true;
            continue;
        }
        return false;
    }

    return has_digit && has_unit;
}

class Scanner
{
  public:
    Scanner(
        const SourceFile& source,
        DiagnosticBag& diagnostics
    )
        : diagnostics_(diagnostics),
          text_(source.text())
    {
    }

    std::vector<Token> scan()
    {
        while (!is_at_end())
        {
            skip_trivia();
            if (is_at_end())
            {
                break;
            }
            scan_token();
        }

        tokens_.push_back(Token{TokenKind::EndOfFile, "", SourceRange{location_, location_}});
        return tokens_;
    }

  private:
    bool is_at_end() const
    {
        return location_.offset >= text_.size();
    }

    char peek(std::size_t offset = 0) const
    {
        const auto index = location_.offset + offset;
        if (index >= text_.size())
        {
            return '\0';
        }
        return text_[index];
    }

    char advance()
    {
        const char ch = peek();
        ++location_.offset;
        if (ch == '\n')
        {
            ++location_.line;
            location_.column = 1;
        }
        else
        {
            ++location_.column;
        }
        return ch;
    }

    bool match(char expected)
    {
        if (peek() != expected)
        {
            return false;
        }
        advance();
        return true;
    }

    void skip_trivia()
    {
        bool consumed = true;
        while (consumed && !is_at_end())
        {
            consumed = false;
            while (!is_at_end())
            {
                const char ch = peek();
                if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
                {
                    advance();
                    consumed = true;
                }
                else
                {
                    break;
                }
            }

            if (peek() == '/' && peek(1) == '/')
            {
                while (!is_at_end() && peek() != '\n')
                {
                    advance();
                }
                consumed = true;
            }
            else if (peek() == '/' && peek(1) == '*')
            {
                const auto start = location_;
                advance();
                advance();
                bool closed = false;
                while (!is_at_end())
                {
                    if (peek() == '*' && peek(1) == '/')
                    {
                        advance();
                        advance();
                        closed = true;
                        break;
                    }
                    advance();
                }

                if (!closed)
                {
                    diagnostics_.error(
                        SourceRange{start, location_}, "SSPEC0101", "unterminated block comment"
                    );
                }
                consumed = true;
            }
        }
    }

    void scan_token()
    {
        const auto start = location_;
        const char ch = advance();

        switch (ch)
        {
        case '{':
            add(TokenKind::LeftBrace, start);
            return;
        case '}':
            add(TokenKind::RightBrace, start);
            return;
        case '(':
            add(TokenKind::LeftParen, start);
            return;
        case ')':
            add(TokenKind::RightParen, start);
            return;
        case '[':
            add(TokenKind::LeftBracket, start);
            return;
        case ']':
            add(TokenKind::RightBracket, start);
            return;
        case ',':
            add(TokenKind::Comma, start);
            return;
        case ':':
            add(TokenKind::Colon, start);
            return;
        case ';':
            add(TokenKind::Semicolon, start);
            return;
        case '.':
            add(TokenKind::Dot, start);
            return;
        case '?':
            add(TokenKind::Question, start);
            return;
        case '+':
            add(TokenKind::Plus, start);
            return;
        case '*':
            add(TokenKind::Star, start);
            return;
        case '%':
            add(TokenKind::Percent, start);
            return;
        case '-':
            if (is_digit(peek()))
            {
                scan_number(start);
            }
            else
            {
                add(match('>') ? TokenKind::Arrow : TokenKind::Minus, start);
            }
            return;
        case '=':
            add(match('=') ? TokenKind::EqualEqual : TokenKind::Equals, start);
            return;
        case '!':
            add(match('=') ? TokenKind::BangEqual : TokenKind::Bang, start);
            return;
        case '<':
            add(match('=') ? TokenKind::LessEqual : TokenKind::Less, start);
            return;
        case '>':
            add(match('=') ? TokenKind::GreaterEqual : TokenKind::Greater, start);
            return;
        case '&':
            if (match('&'))
            {
                add(TokenKind::AndAnd, start);
            }
            else
            {
                unknown(start, "unexpected '&'; did you mean '&&'?");
            }
            return;
        case '|':
            if (match('|'))
            {
                add(TokenKind::OrOr, start);
            }
            else
            {
                unknown(start, "unexpected '|'; did you mean '||'?");
            }
            return;
        case '/':
            add(TokenKind::Slash, start);
            return;
        case '"':
            scan_string(start);
            return;
        default:
            if (is_digit(ch))
            {
                scan_number(start);
                return;
            }
            if (is_identifier_start(ch))
            {
                scan_identifier_or_duration(start);
                return;
            }
            unknown(start, std::string("unexpected character '") + ch + "'");
            return;
        }
    }

    void scan_string(SourceLocation start)
    {
        bool closed = false;
        while (!is_at_end())
        {
            const char ch = advance();
            if (ch == '\\' && !is_at_end())
            {
                advance();
                continue;
            }
            if (ch == '"')
            {
                closed = true;
                break;
            }
        }

        if (!closed)
        {
            diagnostics_.error(
                SourceRange{start, location_}, "SSPEC0102", "unterminated string literal"
            );
        }

        add(TokenKind::StringLiteral, start);
    }

    void scan_number(SourceLocation start)
    {
        while (is_digit(peek()))
        {
            advance();
        }

        TokenKind kind = TokenKind::IntegerLiteral;
        if (peek() == '.' && is_digit(peek(1)))
        {
            kind = TokenKind::DecimalLiteral;
            advance();
            while (is_digit(peek()))
            {
                advance();
            }
        }

        add(kind, start);
    }

    void scan_identifier_or_duration(SourceLocation start)
    {
        while (is_identifier_part(peek()))
        {
            advance();
        }

        const auto lexeme =
            std::string_view{text_}.substr(start.offset, location_.offset - start.offset);
        if (looks_like_duration(lexeme))
        {
            add(TokenKind::DurationLiteral, start);
            return;
        }

        add(keyword_kind(lexeme), start);
    }

    void
    add(TokenKind kind,
        SourceLocation start)
    {
        tokens_.push_back(
            Token{
                kind, text_.substr(start.offset, location_.offset - start.offset),
                SourceRange{start, location_}
            }
        );
    }

    void unknown(
        SourceLocation start,
        const std::string& message
    )
    {
        add(TokenKind::Unknown, start);
        diagnostics_.error(SourceRange{start, location_}, "SSPEC0100", message);
    }

    DiagnosticBag& diagnostics_;
    const std::string& text_;
    SourceLocation location_{};
    std::vector<Token> tokens_;
};

} // namespace

Lexer::Lexer(const SourceFile& source)
    : source_(source)
{
}

std::vector<Token> Lexer::lex(DiagnosticBag& diagnostics)
{
    Scanner scanner{source_, diagnostics};
    return scanner.scan();
}

} // namespace statespec
