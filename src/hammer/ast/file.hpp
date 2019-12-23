#ifndef HAMMER_AST_FILE_HPP
#define HAMMER_AST_FILE_HPP

#include "hammer/ast/node.hpp"
#include "hammer/ast/scope.hpp"

namespace hammer::ast {

/**
 * Represents the content of a single source file.
 */
class File final : public Node, public Scope {
public:
    File();
    ~File();

    InternedString file_name() const { return file_name_; }
    void file_name(InternedString name) { file_name_ = name; }

    size_t item_count() const { return items_.size(); }

    Node* get_item(size_t index) const { return items_.get(index); }

    void add_item(std::unique_ptr<Node> item) {
        item->parent(this);
        items_.push_back(std::move(item));
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Node::visit_children(v);
        items_.for_each(v);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    InternedString file_name_;
    NodeVector<Node> items_;
};

} // namespace hammer::ast

#endif // HAMMER_AST_FILE_HPP
