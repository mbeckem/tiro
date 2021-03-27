#ifndef TIRO_COMPILER_AST_GEN_SCANNER_HPP
#define TIRO_COMPILER_AST_GEN_SCANNER_HPP

#include "compiler/ast_gen/fwd.hpp"
#include "compiler/syntax/syntax_tree.hpp"
#include "compiler/syntax/syntax_type.hpp"
#include "compiler/syntax/token_set.hpp"

#include <algorithm>
#include <optional>

namespace tiro::typed_syntax {

class SyntaxNodeScanner final {
public:
    SyntaxNodeScanner(SyntaxNodeId id, const SyntaxTree& tree)
        : tree_(tree)
        , id_(id)
        , children_(tree_[id].children()) {
        skip_errors();
    }

    bool at_end() const { return pos_ >= size(); }

    size_t size() const { return children_.size(); }

    void advance() {
        if (!at_end()) {
            ++pos_;
            skip_errors();
        }
    }

    std::optional<SyntaxChild> current() const {
        if (pos_ >= children_.size())
            return {};
        return children_[pos_];
    }

    std::optional<Token> accept_token(TokenType type) {
        if (auto c = current(); c && is_token_type(*c, type)) {
            advance();
            return c->as_token();
        }
        return {};
    }

    std::optional<SyntaxNodeId> accept_node(SyntaxType expected, bool skip_tokens = false) {
        if (skip_tokens) {
            find([&](const auto& child) { return is_node(child); });
        }

        if (auto c = current(); c && is_node_type(*c, expected)) {
            advance();
            return c->as_node_id();
        }
        return {};
    }

    template<typename Cond>
    bool find(Cond&& cond) {
        while (1) {
            auto child = current();
            if (!child)
                break;

            if (cond(*child))
                return true;

            advance();
        }

        return false;
    }

    template<typename Cond>
    std::optional<SyntaxChild> search(Cond&& cond) {
        while (1) {
            auto child = current();
            if (!child)
                break;

            advance();

            if (cond(*child))
                return child;
        }

        return {};
    }

    std::optional<Token> search_token() {
        return as_token(search([&](const auto& c) { return is_token(c); }));
    }

    std::optional<Token> search_token(TokenType type) {
        return as_token(search([&](const auto& c) { return is_token_type(c, type); }));
    }

    std::optional<Token> search_token(const TokenSet& types) {
        return as_token(search([&](const auto& c) { return is_token_type_of(c, types); }));
    }

    std::optional<SyntaxNodeId> search_node() {
        return as_node(search([&](const auto& c) { return is_node(c); }));
    }

    std::optional<SyntaxNodeId> search_node(SyntaxType type) {
        return as_node(search([&](const auto& c) { return is_node_type(c, type); }));
    }

    std::optional<SyntaxNodeId> search_node(std::initializer_list<SyntaxType> types) {
        return as_node(search([&](const auto& c) { return is_node_type_of(c, types); }));
    }

    bool is_error(const SyntaxChild& child) const { return is_node_type(child, SyntaxType::Error); }

    bool is_token(const SyntaxChild& child) const { return child.type() == SyntaxChildType::Token; }

    bool is_token_type(const SyntaxChild& child, TokenType type) const {
        return child.type() == SyntaxChildType::Token && child.as_token().type() == type;
    }

    bool is_token_type_of(const SyntaxChild& child, const TokenSet& types) const {
        return child.type() == SyntaxChildType::Token && types.contains(child.as_token().type());
    }

    bool is_node(const SyntaxChild& child) const { return child.type() == SyntaxChildType::NodeId; }

    bool is_node_type(const SyntaxChild& child, SyntaxType type) const {
        return child.type() == SyntaxChildType::NodeId && tree_[child.as_node_id()].type() == type;
    }

    bool is_node_type_of(const SyntaxChild& child, std::initializer_list<SyntaxType> types) const {
        return is_node_type_of(child, Span<const SyntaxType>(types));
    }

    bool is_node_type_of(const SyntaxChild& child, Span<const SyntaxType> types) const {
        if (!is_node(child))
            return false;

        const auto& node_data = tree_[child.as_node_id()];
        return std::find(types.begin(), types.end(), node_data.type()) != types.end();
    }

    static std::optional<SyntaxNodeId> as_node(std::optional<SyntaxChild> child) {
        if (child)
            return child->as_node_id();
        return {};
    }

    static std::optional<Token> as_token(std::optional<SyntaxChild> child) {
        if (child)
            return child->as_token();
        return {};
    }

private:
    void skip_errors() {
        const size_t size = children_.size();
        for (; pos_ < size; ++pos_) {
            auto child = children_[pos_];
            if (!is_error(child))
                break;
        }
    }

private:
    const SyntaxTree& tree_;
    SyntaxNodeId id_;
    Span<const SyntaxChild> children_;
    size_t pos_ = 0;
};

} // namespace tiro::typed_syntax

#endif // TIRO_COMPILER_AST_GEN_SCANNER_HPP
