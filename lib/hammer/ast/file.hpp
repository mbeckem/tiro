#ifndef HAMMER_AST_FILE_HPP
#define HAMMER_AST_FILE_HPP

#include "hammer/ast/node.hpp"
#include "hammer/ast/scope.hpp"

namespace hammer::ast {

/**
 * Represents the content of a single source file.
 */
class File : public Node, public Scope {
public:
    File();
    ~File();

    InternedString file_name() const { return file_name_; }
    void file_name(InternedString name) { file_name_ = name; }

    size_t item_count() const { return items_.size(); }

    Node* get_item(size_t index) const {
        HAMMER_ASSERT(index < items_.size(), "Index out of bounds.");
        return items_[index];
    }

    void add_item(std::unique_ptr<Node> item) { items_.push_back(add_child(std::move(item))); }

    void dump_impl(NodeFormatter& fmt) const;

private:
    InternedString file_name_;
    std::vector<Node*> items_;
};

} // namespace hammer::ast

#endif // HAMMER_AST_FILE_HPP
