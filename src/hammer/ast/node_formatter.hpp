#ifndef HAMMER_AST_NODE_FORMATTER_HPP
#define HAMMER_AST_NODE_FORMATTER_HPP

#include "hammer/ast/fwd.hpp"
#include "hammer/compiler/string_table.hpp"
#include "hammer/core/defs.hpp"

namespace hammer::ast {

class Node;

class NodeFormatter {
public:
    NodeFormatter(
        const StringTable& strings, std::ostream& os, int current_indent = 0);

    // Don't call this directly. Use property() instead.
    void visit_node(const Node& n);

    void property(std::string_view name, bool prop);
    void property(std::string_view name, u64 prop);
    void property(std::string_view name, i64 prop);
    void property(std::string_view name, double prop);
    void property(std::string_view name, const char* prop);
    void property(std::string_view name, std::string_view prop);
    void property(std::string_view name, InternedString prop);
    void property(std::string_view name, const Node& child);
    void property(std::string_view name, const Node* child);

    template<typename Property>
    void properties(std::string_view name, Property&& prop) {
        property(name, std::forward<Property>(prop));
    }

    template<typename Property, typename... Rest>
    void properties(std::string_view name, Property&& prop, Rest&&... rest) {
        property(name, std::forward<Property>(prop));
        properties(std::forward<Rest>(rest)...);
    }

    const StringTable& strings() const { return strings_; }

private:
    std::ostream& prop_name(std::string_view name);
    std::ostream& line();

private:
    const StringTable& strings_;
    std::ostream& os_;
    int current_indent_ = 0;
};

} // namespace hammer::ast

#endif // HAMMER_AST_NODE_FORMATTER_HPP
