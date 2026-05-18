#pragma once

#include "statespec/token.hpp"

#include <exception>
#include <string>

namespace statespec::parser_detail
{

inline int parse_int_or_zero(const Token& token)
{
    try
    {
        return std::stoi(token.lexeme);
    }
    catch (const std::exception&)
    {
        return 0;
    }
}

inline std::string strip_quotes(const std::string& value)
{
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
    {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

inline bool is_named_identifier(
    const Token& token,
    const std::string& name
)
{
    return token.kind == TokenKind::Identifier && token.lexeme == name;
}

inline std::string qualify_name(
    const std::string& prefix,
    const std::string& name
)
{
    if (prefix.empty())
    {
        return name;
    }
    return prefix + "." + name;
}

template <typename T>
void qualify_decl_name(
    T& decl,
    const std::string& prefix
)
{
    decl.name = qualify_name(prefix, decl.name);
}

} // namespace statespec::parser_detail
