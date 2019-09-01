#include "hammer/ast/node_formatter.hpp"

#include "hammer/ast/node.hpp"
#include "hammer/ast/node_visit.hpp"

#include <ostream>

namespace hammer::ast {

NodeFormatter::NodeFormatter(
    const StringTable& strings, std::ostream& os, int current_indent)
    : strings_(strings)
    , os_(os)
    , current_indent_(current_indent) {}

void NodeFormatter::property(std::string_view name, bool prop) {
    prop_name(name) << (prop ? "true" : "false") << "\n";
}

void NodeFormatter::property(std::string_view name, u64 prop) {
    prop_name(name) << prop << "\n";
}

void NodeFormatter::property(std::string_view name, i64 prop) {
    prop_name(name) << prop << "\n";
}

void NodeFormatter::property(std::string_view name, double prop) {
    prop_name(name) << prop << "\n";
}

void NodeFormatter::property(std::string_view name, const char* prop) {
    prop_name(name) << prop << "\n";
}

void NodeFormatter::property(std::string_view name, std::string_view prop) {
    prop_name(name) << prop << "\n";
}

void NodeFormatter::property(std::string_view name, InternedString prop) {
    prop_name(name) << (prop.valid() ? strings_.value(prop) : "") << "\n";
}

void NodeFormatter::property(std::string_view name, const Node& child) {
    prop_name(name) << "\n";

    ++current_indent_;
    visit_node(child);
    --current_indent_;
}

void NodeFormatter::property(std::string_view name, const Node* child) {
    if (!child) {
        prop_name(name) << "null\n";
    } else {
        property(name, *child);
    }
}

void NodeFormatter::visit_node(const Node& n) {
    line() << to_string(n.kind()) << "\n";

    ++current_indent_;
    visit(n, [&](const auto& node) { return node.dump_impl(*this); });
    --current_indent_;
}

std::ostream& NodeFormatter::prop_name(std::string_view name) {
    return line() << name << ": ";
}

std::ostream& NodeFormatter::line() {
    for (int i = 0; i < current_indent_; ++i) {
        os_ << "  ";
    }
    return os_;
}

} // namespace hammer::ast
