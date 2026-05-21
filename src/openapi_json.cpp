#include "openapi_json.hpp"

#include <sstream>

namespace statespec
{

std::string openapi_json_escape(const std::string& value)
{
    std::ostringstream out;
    for (const char ch : value)
    {
        switch (ch)
        {
        case '"':
            out << "\\\"";
            break;
        case '\\':
            out << "\\\\";
            break;
        case '\b':
            out << "\\b";
            break;
        case '\f':
            out << "\\f";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            const auto uch = static_cast<unsigned char>(ch);
            if (uch < 0x20)
            {
                out << "\\u00";
                constexpr char digits[] = "0123456789abcdef";
                out << digits[(uch >> 4) & 0x0f] << digits[uch & 0x0f];
            }
            else
            {
                out << ch;
            }
            break;
        }
    }
    return out.str();
}

std::string openapi_quoted(const std::string& value)
{
    return "\"" + openapi_json_escape(value) + "\"";
}

} // namespace statespec
