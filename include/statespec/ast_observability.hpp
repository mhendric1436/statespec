#pragma once

#include "statespec/ast_core.hpp"

namespace statespec
{

struct LogDecl
{
    std::string name;
    std::optional<std::string> level;
    std::optional<std::string> event_name;
    std::vector<FieldDecl> fields;
    std::vector<BlockMemberOrder> member_order;
    SourceRange range;
};

struct MetricDecl
{
    std::string name;
    std::optional<std::string> kind;
    std::optional<std::string> backend_name;
    std::optional<std::string> unit;
    std::vector<FieldDecl> labels;
    std::vector<BlockMemberOrder> member_order;
    SourceRange range;
};

} // namespace statespec
