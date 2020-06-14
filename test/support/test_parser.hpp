#ifndef TIRO_TEST_TEST_PARSER
#define TIRO_TEST_TEST_PARSER

#include <catch.hpp>

#include "compiler/parser/parser.hpp"

namespace tiro {

class TestParser {
public:
    explicit TestParser()
        : diag_()
        , strings_() {}

    virtual ~TestParser() = default;

    TestParser(const TestParser&) = delete;
    TestParser& operator=(const TestParser&) = delete;

    Diagnostics& diag() { return diag_; }
    StringTable& strings() { return strings_; }

    std::string dump_diag() {
        if (diag().message_count() == 0)
            return {};

        StringFormatStream stream;
        for (const auto& message : diag().messages()) {
            stream.format(
                "{} (at offset {}): {}\n", message.level, message.source.begin(), message.text);
        }
        return stream.take_str();
    }

    AstPtr<AstFile> parse_file(std::string_view source) {
        return unwrap(parser(source).parse_file());
    }

    AstPtr<AstNode> parse_toplevel_item(std::string_view source) {
        return unwrap(parser(source).parse_item({}));
    }

    AstPtr<AstStmt> parse_stmt(std::string_view source) {
        return unwrap(parser(source).parse_stmt({}));
    }

    AstPtr<AstExpr> parse_expr(std::string_view source) {
        return unwrap(parser(source).parse_expr({}));
    }

    std::string_view value(InternedString str) const {
        REQUIRE(str);
        return strings_.value(str);
    }

private:
    Parser parser(std::string_view source) { return Parser{"unit-test", source, strings_, diag_}; }

    template<typename T>
    AstPtr<T> unwrap(Parser::Result<T> result) {
        if (diag_.message_count() > 0) {
            for (const auto& msg : diag_.messages()) {
                UNSCOPED_INFO(msg.text);
            }

            FAIL();
        }
        REQUIRE(result.is_ok());
        REQUIRE(result.has_node());

        auto node = result.take_node();
        REQUIRE(node);
        REQUIRE(!node->has_error());
        return node;
    }

private:
    Diagnostics diag_;
    StringTable strings_;
};

} // namespace tiro

#endif // TIRO_TEST_TEST_PARSER
