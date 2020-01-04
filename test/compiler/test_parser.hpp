#ifndef HAMMER_COMPILER_TEST_PARSER
#define HAMMER_COMPILER_TEST_PARSER

#include <catch.hpp>

#include "hammer/compiler/syntax/parser.hpp"

namespace hammer::compiler {

class TestParser {
public:
    explicit TestParser()
        : diag_()
        , strings_() {}

    TestParser(const TestParser&) = delete;
    TestParser& operator=(const TestParser&) = delete;

    Diagnostics& diag() { return diag_; }
    StringTable& strings() { return strings_; }

    NodePtr<File> parse_file(std::string_view source) {
        return unwrap(parser(source).parse_file());
    }

    NodePtr<Node> parse_toplevel_item(std::string_view source) {
        return unwrap(parser(source).parse_toplevel_item({}));
    }

    NodePtr<Stmt> parse_stmt(std::string_view source) {
        return unwrap(parser(source).parse_stmt({}));
    }

    NodePtr<Expr> parse_expr(std::string_view source) {
        return unwrap(parser(source).parse_expr({}));
    }

    std::string_view value(InternedString str) const {
        REQUIRE(str);
        return strings_.value(str);
    }

private:
    Parser parser(std::string_view source) {
        return Parser{"unit-test", source, strings_, diag_};
    }

    template<typename T>
    NodePtr<T> unwrap(Parser::Result<T> result) {
        if (diag_.message_count() > 0) {
            for (const auto& msg : diag_.messages()) {
                UNSCOPED_INFO(msg.text);
            }

            FAIL();
        }
        REQUIRE(result);

        auto node = result.take_node();
        REQUIRE(node);
        REQUIRE(!node->has_error());
        return node;
    }

private:
    Diagnostics diag_;
    StringTable strings_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_TEST_PARSER
