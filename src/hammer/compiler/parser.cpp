#include "hammer/compiler/parser.hpp"

#include "hammer/ast/decl.hpp"
#include "hammer/ast/expr.hpp"
#include "hammer/ast/file.hpp"
#include "hammer/ast/literal.hpp"
#include "hammer/ast/stmt.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/math.hpp"

#include <fmt/format.h>

namespace hammer {

template<typename Derived, typename Base>
std::unique_ptr<Derived> node_downcast(std::unique_ptr<Base> node) {
    HAMMER_ASSERT_NOT_NULL(node);
    return std::unique_ptr<Derived>(must_cast<Derived>(node.release()));
}

static int infix_operator_precedence(TokenType t) {
    switch (t) {
    // Assigment
    case TokenType::Equals:
        return 0;

    case TokenType::LogicalOr:
        return 1;

    case TokenType::LogicalAnd:
        return 2;

    case TokenType::BitwiseOr:
        return 3;

    case TokenType::BitwiseXor:
        return 4;

    case TokenType::BitwiseAnd:
        return 5;

    // TODO Reconsider precendence of equality: should it be lower than Bitwise xor/or/and?
    case TokenType::EqualsEquals:
    case TokenType::NotEquals:
        return 6;

    case TokenType::Less:
    case TokenType::LessEquals:
    case TokenType::Greater:
    case TokenType::GreaterEquals:
        return 7;

    case TokenType::LeftShift:
    case TokenType::RightShift:
        return 8;

    case TokenType::Plus:
    case TokenType::Minus:
        return 9;

    case TokenType::Star:    // Multiply
    case TokenType::Slash:   // Divide
    case TokenType::Percent: // Modulus
        return 10;

    case TokenType::StarStar: // Power
        return 11;

        // UNARY OPERATORS == 12

    case TokenType::LeftParen:   // Function call
    case TokenType::LeftBracket: // Array
    case TokenType::Dot:         // Member access
        return 13;

    default:
        return -1;
    }
}

static constexpr int UNARY_PRECEDENCE = 12;

static std::optional<ast::UnaryOperator> to_unary_operator(TokenType t) {
    switch (t) {
    case TokenType::Plus:
        return ast::UnaryOperator::Plus;
    case TokenType::Minus:
        return ast::UnaryOperator::Minus;
    case TokenType::LogicalNot:
        return ast::UnaryOperator::LogicalNot;
    case TokenType::BitwiseNot:
        return ast::UnaryOperator::BitwiseNot;
    default:
        return {};
    }
}

static std::optional<ast::BinaryOperator> to_binary_operator(TokenType t) {
#define HAMMER_MAP_TOKEN(token, op) \
    case TokenType::token:          \
        return ast::BinaryOperator::op;

    switch (t) {
        HAMMER_MAP_TOKEN(Plus, Plus)
        HAMMER_MAP_TOKEN(Minus, Minus)
        HAMMER_MAP_TOKEN(Star, Multiply)
        HAMMER_MAP_TOKEN(Slash, Divide)
        HAMMER_MAP_TOKEN(Percent, Modulus)
        HAMMER_MAP_TOKEN(StarStar, Power)
        HAMMER_MAP_TOKEN(LeftShift, LeftShift)
        HAMMER_MAP_TOKEN(RightShift, RightShift)

        HAMMER_MAP_TOKEN(BitwiseAnd, BitwiseAnd)
        HAMMER_MAP_TOKEN(BitwiseOr, BitwiseOr)
        HAMMER_MAP_TOKEN(BitwiseXor, BitwiseXor)

        HAMMER_MAP_TOKEN(Less, Less)
        HAMMER_MAP_TOKEN(LessEquals, LessEquals)
        HAMMER_MAP_TOKEN(Greater, Greater)
        HAMMER_MAP_TOKEN(GreaterEquals, GreaterEquals)
        HAMMER_MAP_TOKEN(EqualsEquals, Equals)
        HAMMER_MAP_TOKEN(NotEquals, NotEquals)
        HAMMER_MAP_TOKEN(LogicalAnd, LogicalAnd)
        HAMMER_MAP_TOKEN(LogicalOr, LogicalOr)

        HAMMER_MAP_TOKEN(Equals, Assign)

    default:
        return {};
    }

#undef HAMMER_MAP_TOKEN
}

static bool operator_is_right_associative(ast::BinaryOperator op) {
    using ast::BinaryOperator;

    switch (op) {
    case BinaryOperator::Assign:
    case BinaryOperator::Power:
        return true;
    default:
        return false;
    }
}

static std::string unexpected_message(
    std::string_view context, TokenTypes expected, TokenType seen) {
    const size_t size = expected.size();

    fmt::memory_buffer buf;
    if (!context.empty()) {
        fmt::format_to(
            buf, "Unexpected {} in {} context", to_description(seen), context);
    } else {
        fmt::format_to(buf, "Unexpected {}", to_description(seen));
    }

    if (size > 0 && size <= 3) {
        fmt::format_to(buf, ", expected ");

        size_t index = 0;
        for (TokenType ex : expected) {
            if (index != 0)
                fmt::format_to(buf, index + 1 == size ? " or " : ", ");

            fmt::format_to(buf, "{}", to_description(ex));
            ++index;
        }
    }

    fmt::format_to(buf, ".");
    return to_string(buf);
}

// Important: all token types that can be a legal beginning of an expression
// MUST be listed here. Otherwise, the expression parser will bail out immediately,
// even if the token would be handled somewhere down in the implementation!
static const TokenTypes EXPR_FIRST = {
    // Keywords
    TokenType::KwFunc,
    TokenType::KwContinue,
    TokenType::KwBreak,
    TokenType::KwReturn,
    TokenType::KwIf,
    TokenType::KwMap,
    TokenType::KwSet,

    // Literal constants
    TokenType::KwTrue,
    TokenType::KwFalse,
    TokenType::KwNull,

    // Literal values
    TokenType::Identifier,
    TokenType::SymbolLiteral,
    TokenType::StringLiteral,
    TokenType::FloatLiteral,
    TokenType::IntegerLiteral,

    // ( expr ) either a braced expr or a tuple
    TokenType::LeftParen,

    // Array
    TokenType::LeftBracket,

    // { statements ... }
    TokenType::LeftBrace,

    // Unary operators
    TokenType::Plus,
    TokenType::Minus,
    TokenType::BitwiseNot,
    TokenType::LogicalNot,
};

static const TokenTypes VAR_DECL_FIRST = {
    TokenType::KwVar,
    TokenType::KwConst,
};

static const TokenTypes STMT_FIRST =
    TokenTypes{
        TokenType::Semicolon,
        TokenType::KwAssert,
        TokenType::KwWhile,
        TokenType::KwFor,
    }
        .union_with(VAR_DECL_FIRST)
        .union_with(EXPR_FIRST);

static const TokenTypes TOPLEVEL_ITEM_FIRST = {
    TokenType::KwImport, TokenType::KwFunc, TokenType::Semicolon,
    // TODO Export
};

static const TokenTypes EXPR_STMT_OPTIONAL_SEMICOLON = {
    TokenType::KwFunc,
    TokenType::KwIf,
    TokenType::LeftBrace,
};

template<typename Node, typename... Args>
std::unique_ptr<Node> Parser::make_node(const Token& start, Args&&... args) {
    static_assert(std::is_base_of_v<ast::Node, Node>,
        "Node must be a derived class of ast::Node.");

    auto node = std::make_unique<Node>(std::forward<Args>(args)...);
    node->start(start.source());
    if (start.has_error())
        node->has_error(true);
    return node;
}

template<typename Node>
Parser::Result<Node>
Parser::result(std::unique_ptr<Node>&& node, bool parse_ok) {
    if (!node)
        return error(std::move(node));
    if (!parse_ok)
        return error(std::move(node));
    return Result<Node>(std::move(node));
}

template<typename Node>
Parser::Result<Node> Parser::error(std::unique_ptr<Node>&& node) {
    static_assert(std::is_base_of_v<ast::Node, Node>);

    if (node)
        node->has_error(true);
    return Result<Node>(std::move(node), false);
}

Parser::ErrorTag Parser::error() {
    return ErrorTag();
}

template<typename Node, typename OtherNode>
Parser::Result<Node>
Parser::forward(std::unique_ptr<Node>&& node, const Result<OtherNode>& other) {
    static_assert(std::is_base_of_v<ast::Node, Node>);

    const bool ok = static_cast<bool>(other);
    if (node && !ok)
        node->has_error(true);
    return Result<Node>(std::move(node), ok);
}

template<typename SubParser>
bool Parser::parse_braced_list(
    const ListOptions& options, TokenTypes sync, SubParser&& parser) {
    HAMMER_ASSERT(!options.name.empty(), "Must not have an empty name.");
    HAMMER_ASSERT(options.right_brace != TokenType::InvalidToken,
        "Must set the right brace token type.");
    HAMMER_ASSERT(options.max_count == -1 || options.max_count >= 0,
        "Invalid max count.");

    int current_count = 0;

    if (accept(options.right_brace))
        return true;

    const auto inner_sync = sync.union_with(
        {TokenType::Comma, options.right_brace});

    while (1) {
        if (current_.type() == TokenType::Eof) {
            diag_.reportf(Diagnostics::Error, current_.source(),
                "Unterminated {}, expected {}.", options.name,
                to_description(options.right_brace));
            return false;
        }

        if (options.max_count != -1 && current_count >= options.max_count) {
            // TODO: Proper recovery until "," or brace?
            diag_.reportf(Diagnostics::Error, current_.source(),
                "Unexpected {} in {}, expected {}.",
                to_description(current_.type()), options.name,
                to_description(options.right_brace));
            return false;
        }

        // Call the sub parser.
        const bool parser_ok = parser(inner_sync);
        ++current_count;

        // On success, we expect "," or closing brace.
        std::optional<Token> next;
        if (parser_ok)
            next = expect({TokenType::Comma, options.right_brace});

        // Either parser failed or expect failed
        if (!next) {
            if (!(next = recover_consume(
                      {TokenType::Comma, options.right_brace}, sync)))
                return false; // Recovery failed
        }

        if (next->type() == options.right_brace)
            return true;

        if (next->type() == TokenType::Comma) {
            // Trailing comma
            if (options.allow_trailing_comma && accept(options.right_brace))
                return true;
            continue;
        }

        HAMMER_UNREACHABLE("Invalid token type.");
    }
}

Parser::Parser(std::string_view file_name, std::string_view source,
    StringTable& strings, Diagnostics& diag)
    : file_name_(strings.insert(file_name))
    , source_(source)
    , strings_(strings)
    , diag_(diag)
    , lexer_(file_name_, source_, strings_, diag_) {

    advance();
}

Parser::Result<ast::File> Parser::parse_file() {
    std::unique_ptr<ast::File> file = make_node<ast::File>(current_);
    file->file_name(file_name_);

    while (!accept(TokenType::Eof)) {
        if (auto brace = accept({TokenType::RightBrace, TokenType::RightBracket,
                TokenType::RightParen})) {
            diag_.reportf(Diagnostics::Error, brace->source(), "Unbalanced {}.",
                to_description(brace->type()));
            continue;
        }

        auto item = parse_toplevel_item({});
        item.with_node([&](auto&& node) { file->add_item(std::move(node)); });
        if (!item) {
            if (!recover_seek(TOPLEVEL_ITEM_FIRST, {}))
                return error(std::move(file));
        }
    }

    return file;
}

Parser::Result<ast::Node> Parser::parse_toplevel_item(TokenTypes sync) {
    switch (current_.type()) {
    case TokenType::KwImport:
        return parse_import_decl(sync);
    case TokenType::KwFunc:
        return parse_func_decl(true, sync);
    case TokenType::Semicolon: {
        auto node = make_node<ast::EmptyStmt>(current_);
        advance();
        return node;
    }
    default:
        diag_.reportf(Diagnostics::Error, current_.source(), "Unexpected {}.",
            to_description(current_.type()));
        return error();
    }
}

Parser::Result<ast::ImportDecl> Parser::parse_import_decl(TokenTypes sync) {
    auto start_tok = expect(TokenType::KwImport);
    if (!start_tok)
        return error();

    auto decl = make_node<ast::ImportDecl>(*start_tok);

    auto ident = expect(TokenType::Identifier);
    if (!ident)
        goto recover;

    decl->name(ident->string_value());
    if (ident->has_error())
        goto recover;

    if (!expect(TokenType::Semicolon))
        goto recover;

    return decl;

recover:
    decl->has_error(true);
    const bool ok = recover_consume(TokenType::Semicolon, sync).has_value();
    return result(std::move(decl), ok);
}

Parser::Result<ast::FuncDecl>
Parser::parse_func_decl(bool requires_name, TokenTypes sync) {
    auto start_tok = expect(TokenType::KwFunc);
    if (!start_tok)
        return error();

    auto func = make_node<ast::FuncDecl>(*start_tok);

    if (auto ident = accept(TokenType::Identifier)) {
        func->name(ident->string_value());
        if (ident->has_error()) {
            func->has_error(true);
        }
    } else if (requires_name) {
        diag_.reportf(Diagnostics::Error, current_.source(),
            "Expected a valid identifier for the new function's name but "
            "saw a {} instead.",
            to_description(current_.type()));
        func->has_error(true);
    }

    if (!expect(TokenType::LeftParen))
        return error(std::move(func));

    static constexpr ListOptions options{
        "parameter list", TokenType::RightParen};

    const bool list_ok = parse_braced_list(
        options, sync, [&]([[maybe_unused]] TokenTypes inner_sync) {
            auto param_ident = expect(TokenType::Identifier);
            if (param_ident) {
                auto param = make_node<ast::ParamDecl>(*param_ident);
                param->name(param_ident->string_value());
                func->add_param(std::move(param));
                return true;
            } else {
                return false;
            }
        });
    if (!list_ok)
        return error(std::move(func));

    auto body = parse_block_expr(sync);
    func->body(body.take_node());
    return forward(std::move(func), body);
}

Parser::Result<ast::Stmt> Parser::parse_stmt(TokenTypes sync) {
    if (auto empty_tok = accept(TokenType::Semicolon))
        return make_node<ast::EmptyStmt>(*empty_tok);

    const TokenType type = current_.type();

    if (type == TokenType::KwAssert) {
        // TODO merge recovery code with var decl and expr stmt
        auto stmt = parse_assert(sync.union_with(TokenType::Semicolon));
        if (!stmt)
            goto recover_assert;
        if (!expect(TokenType::Semicolon))
            goto recover_assert;
        return stmt;

    recover_assert:
        if (recover_consume(TokenType::Semicolon, sync))
            return stmt;
        return error(stmt.take_node());
    }

    if (type == TokenType::KwWhile) {
        auto stmt = parse_while_stmt(sync);
        accept(TokenType::Semicolon);
        return stmt;
    }

    if (type == TokenType::KwFor) {
        auto stmt = parse_for_stmt(sync);
        accept(TokenType::Semicolon);
        return stmt;
    }

    if (can_begin_var_decl(type)) {
        auto stmt = parse_var_decl(sync.union_with(TokenType::Semicolon));
        if (!stmt)
            goto recover_decl;
        if (!expect(TokenType::Semicolon))
            goto recover_decl;
        return stmt;

    recover_decl:
        if (recover_consume(TokenType::Semicolon, sync))
            return stmt;
        return error(stmt.take_node());
    }

    if (can_begin_expression(type)) {
        return parse_expr_stmt(sync);
    }

    // Hint: can_begin_expression could be out of sync with
    // the expression parser.
    diag_.reportf(Diagnostics::Error, current_.source(),
        "Unexpected {} in statement context.", to_description(type));
    return error();
}

Parser::Result<ast::AssertStmt> Parser::parse_assert(TokenTypes sync) {
    auto start_tok = expect(TokenType::KwAssert);
    if (!start_tok)
        return error();

    auto stmt = make_node<ast::AssertStmt>(*start_tok);

    if (!expect(TokenType::LeftParen))
        return error(std::move(stmt));

    // TODO min args?
    static constexpr auto options = ListOptions(
        "assertion statement", TokenType::RightParen)
                                        .set_max_count(2);

    int argument = 0;
    const bool args_ok = parse_braced_list(
        options, sync, [&](TokenTypes inner_sync) mutable {
            switch (argument++) {
            // Condition
            case 0: {
                auto expr = parse_expr(inner_sync);
                if (expr.has_node())
                    stmt->condition(expr.take_node());
                return expr.parse_ok();
            }

            // Optional message
            case 1: {
                auto expr = parse_expr(inner_sync);
                if (auto node = expr.take_node()) {
                    if (isa<ast::StringLiteral>(node.get())) {
                        stmt->message(
                            node_downcast<ast::StringLiteral>(std::move(node)));
                    } else {
                        diag_.reportf(Diagnostics::Error, node->start(),
                            "Expected a string literal.",
                            to_string(node->kind()));
                        // Continue parsing, this is ok ..
                    }
                }
                return expr.parse_ok();
            }
            default:
                HAMMER_UNREACHABLE(
                    "Assertion argument parser called too often.");
            }
        });

    if (argument < 1) {
        diag_.report(Diagnostics::Error, start_tok->source(),
            "Assertion must have at least one argument.");
        stmt->has_error(true);
    }

    if (args_ok)
        return stmt;
    return error(std::move(stmt));
}

Parser::Result<ast::DeclStmt> Parser::parse_var_decl(TokenTypes sync) {
    const auto decl_tok = expect(VAR_DECL_FIRST);
    if (!decl_tok)
        return error();

    auto decl = make_node<ast::DeclStmt>(*decl_tok);

    auto ident = accept(TokenType::Identifier);
    if (!ident) {
        diag_.reportf(Diagnostics::Error, current_.source(),
            "Unexpected {}, expected a valid identifier.",
            to_description(current_.type()));
        return error(std::move(decl));
    }

    decl->declaration(make_node<ast::VarDecl>(*ident));

    ast::VarDecl* var = decl->declaration();
    var->is_const(decl_tok->type() == TokenType::KwConst);
    var->name(ident->string_value());

    if (ident->has_error())
        return error(std::move(decl));

    if (!accept(TokenType::Equals))
        return decl;

    auto expr = parse_expr(sync);
    var->initializer(expr.take_node());
    if (!expr)
        return error(std::move(decl));

    return decl;
}

Parser::Result<ast::WhileStmt> Parser::parse_while_stmt(TokenTypes sync) {
    auto start_tok = expect(TokenType::KwWhile);
    if (!start_tok)
        return error();

    auto stmt = make_node<ast::WhileStmt>(*start_tok);

    auto cond = parse_expr(sync.union_with(TokenType::LeftBrace));
    stmt->condition(cond.take_node());
    if (!cond)
        stmt->has_error(true);

    if (current_.type() != TokenType::LeftBrace) {
        recover_seek(TokenType::LeftBrace, sync);
        stmt->has_error(true);
    }

    auto body = parse_block_expr(sync);
    stmt->body(body.take_node());
    if (!body)
        stmt->has_error(true);

    return forward(std::move(stmt), body);
}

Parser::Result<ast::ForStmt> Parser::parse_for_stmt(TokenTypes sync) {
    auto start_tok = expect(TokenType::KwFor);
    if (!start_tok)
        return error();

    auto stmt = make_node<ast::ForStmt>(*start_tok);

    const auto parse_header = [&] {
        const bool has_parens = static_cast<bool>(accept(TokenType::LeftParen));
        if (!parse_for_stmt_header(stmt.get(), has_parens,
                has_parens ? sync.union_with(TokenType::RightParen) : sync))
            goto recover;

        if (has_parens && !expect(TokenType::RightParen))
            goto recover;

        return true;

    recover:
        stmt->has_error(true);
        return has_parens
                   ? recover_consume(TokenType::RightParen, sync).has_value()
                   : false;
    };

    if (!parse_header())
        return error(std::move(stmt));

    // Loop body
    auto body = parse_block_expr(sync);
    stmt->body(body.take_node());
    return forward(std::move(stmt), body);
}

bool Parser::parse_for_stmt_header(
    ast::ForStmt* stmt, bool has_parens, TokenTypes sync) {

    const auto parse_init = [&] {
        if (!can_begin_var_decl(current_.type())) {
            diag_.reportf(Diagnostics::Error, current_.source(),
                "Expected a variable declaration or a {}.",
                to_description(TokenType::Semicolon));
            goto recover;
        }

        {
            auto decl = parse_var_decl(sync.union_with(TokenType::Semicolon));
            stmt->decl(decl.take_node());
            if (!decl)
                goto recover;
        }

        if (!expect(TokenType::Semicolon))
            goto recover;

        return true;

    recover:
        stmt->has_error(true);
        return recover_consume(TokenType::Semicolon, sync).has_value();
    };

    const auto parse_condition = [&] {
        auto expr = parse_expr(sync.union_with(TokenType::Semicolon));
        stmt->condition(expr.take_node());
        if (!expr)
            goto recover;

        if (!expect(TokenType::Semicolon))
            goto recover;

        return true;

    recover:
        stmt->has_error(true);
        return recover_consume(TokenType::Semicolon, sync).has_value();
    };

    const auto parse_step = [&] {
        auto expr = parse_expr(sync);
        stmt->step(expr.take_node());
        if (!expr)
            goto recover;

        return true;

    recover:
        stmt->has_error(true);
        return false; /* no recovery here, go to caller */
    };

    // Optional init statement
    if (!accept(TokenType::Semicolon)) {
        if (!parse_init())
            return false;
    }

    // Optional condition expression
    if (!accept(TokenType::Semicolon)) {
        if (!parse_condition())
            return false;
    }

    // Optional step expression
    if (has_parens ? current_.type() != TokenType::RightParen
                   : current_.type() != TokenType::LeftBrace) {
        if (!parse_step())
            return false;
    }

    return true;
}

Parser::Result<ast::ExprStmt> Parser::parse_expr_stmt(TokenTypes sync) {
    const bool need_semicolon = !EXPR_STMT_OPTIONAL_SEMICOLON.contains(
        current_.type());

    auto stmt = make_node<ast::ExprStmt>(current_);

    auto expr = parse_expr(sync.union_with(TokenType::Semicolon));
    stmt->expression(expr.take_node());
    if (!expr)
        goto recover;

    if (need_semicolon) {
        if (!expect(TokenType::Semicolon))
            goto recover;
    } else {
        accept(TokenType::Semicolon);
    }

    return stmt;

recover:
    stmt->has_error(true);
    if (recover_consume(TokenType::Semicolon, sync))
        return stmt;
    return error(std::move(stmt));
}

Parser::Result<ast::Expr> Parser::parse_expr(TokenTypes sync) {
    return parse_expr(0, sync);
}

/*
 * Recursive function that implements a pratt parser.
 *
 * See also:
 *      http://crockford.com/javascript/tdop/tdop.html
 *      https://www.oilshell.org/blog/2016/11/01.html
 *      https://groups.google.com/forum/#!topic/comp.compilers/ruJLlQTVJ8o
 */
Parser::Result<ast::Expr>
Parser::parse_expr(int min_precedence, TokenTypes sync) {
    auto left = parse_prefix_expr(sync);
    if (!left)
        return left;

    while (1) {
        const int op_precedence = infix_operator_precedence(current_.type());
        if (op_precedence == -1)
            break; // Not an infix operator.

        if (op_precedence < min_precedence)
            break; // Upper call will handle lower precedence

        left = parse_infix_expr(left.take_node(), op_precedence, sync);
        if (!left)
            break;
    }

    return left;
}

Parser::Result<ast::Expr> Parser::parse_infix_expr(
    std::unique_ptr<ast::Expr> left, int current_precedence, TokenTypes sync) {

    if (auto op = to_binary_operator(current_.type())) {
        auto binary_expr = make_node<ast::BinaryExpr>(current_, *op);
        advance();
        binary_expr->left_child(std::move(left));

        int next_precedence = current_precedence;
        if (!operator_is_right_associative(*op))
            ++next_precedence;

        auto right = parse_expr(next_precedence, sync);
        binary_expr->right_child(right.take_node());
        return forward(std::move(binary_expr), right);
    } else if (current_.type() == TokenType::LeftParen) {
        return parse_call_expr(std::move(left), sync);
    } else if (current_.type() == TokenType::LeftBracket) {
        return parse_index_expr(std::move(left), sync);
    } else if (current_.type() == TokenType::Dot) {
        return parse_member_expr(std::move(left), sync);
    } else {
        HAMMER_ERROR("Invalid operator in parse_infix_operator: {}",
            to_description(current_.type()));
    }
}

/*
 * Parses a unary expressions. Unary expressions are either plain primary
 * expressions or a unary operator followed by another unary expression.
 */
Parser::Result<ast::Expr> Parser::parse_prefix_expr(TokenTypes sync) {
    auto op = to_unary_operator(current_.type());
    if (!op)
        return parse_primary_expr(sync);

    // It's a unary operator
    auto unary = make_node<ast::UnaryExpr>(current_, *op);
    advance();

    auto inner = parse_expr(UNARY_PRECEDENCE, sync);
    unary->inner(inner.take_node());
    return forward(std::move(unary), inner);
}

Parser::Result<ast::DotExpr> Parser::parse_member_expr(
    std::unique_ptr<ast::Expr> current, [[maybe_unused]] TokenTypes sync) {
    auto start_tok = expect(TokenType::Dot);
    if (!start_tok)
        return error();

    auto dot = make_node<ast::DotExpr>(*start_tok);
    dot->inner(std::move(current));

    if (auto ident_tok = expect(TokenType::Identifier)) {
        dot->name(ident_tok->string_value());
        if (ident_tok->has_error())
            return error(std::move(dot));

    } else {
        return error(std::move(dot));
    }

    return dot;
}

Parser::Result<ast::CallExpr>
Parser::parse_call_expr(std::unique_ptr<ast::Expr> current, TokenTypes sync) {
    auto start_tok = expect(TokenType::LeftParen);
    if (!start_tok)
        return error();

    auto call = make_node<ast::CallExpr>(*start_tok);
    call->func(std::move(current));

    static constexpr ListOptions options{
        "argument list", TokenType::RightParen};
    const bool list_ok = parse_braced_list(
        options, sync, [&](TokenTypes inner_sync) {
            auto arg = parse_expr(inner_sync);
            arg.with_node([&](auto&& node) { call->add_arg(std::move(node)); });
            return arg.parse_ok();
        });

    return result(std::move(call), list_ok);
}

Parser::Result<ast::IndexExpr>
Parser::parse_index_expr(std::unique_ptr<ast::Expr> current, TokenTypes sync) {
    auto start_tok = expect(TokenType::LeftBracket);
    if (!start_tok)
        return error();

    auto expr = make_node<ast::IndexExpr>(*start_tok);
    expr->inner(std::move(current));

    auto index = parse_expr(TokenType::RightBracket);
    expr->index(index.take_node());
    if (!index)
        goto recover;

    if (!expect(TokenType::RightBracket))
        goto recover;

    return expr;

recover:
    expr->has_error(true);
    if (recover_consume(TokenType::RightBracket, sync))
        return expr;
    return error(std::move(expr));
}

Parser::Result<ast::Expr> Parser::parse_primary_expr(TokenTypes sync) {
    switch (current_.type()) {

    // Block expr
    case TokenType::LeftBrace: {
        return parse_block_expr(sync);
    }

    // Braced subexpression
    case TokenType::LeftParen: {
        return parse_paren_expr(sync);
    }

    // If expression
    case TokenType::KwIf: {
        return parse_if_expr(sync);
    }

    // Return expression
    case TokenType::KwReturn: {
        auto ret = make_node<ast::ReturnExpr>(current_);
        advance();

        if (can_begin_expression(current_.type())) {
            auto inner = parse_expr(sync);
            ret->inner(inner.take_node());
            if (!inner)
                return error(std::move(ret));
        }
        return ret;
    }

    // Continue expression
    case TokenType::KwContinue: {
        auto cont = make_node<ast::ContinueExpr>(current_);
        advance();
        return cont;
    }

    // Break expression
    case TokenType::KwBreak: {
        auto brk = make_node<ast::BreakExpr>(current_);
        advance();
        return brk;
    }

    // Variable reference
    case TokenType::Identifier: {
        const bool has_error = current_.has_error();
        auto id = make_node<ast::VarExpr>(current_, current_.string_value());
        advance();
        return result(std::move(id), !has_error);
    }

    // Function Literal
    case TokenType::KwFunc: {
        auto ret = make_node<ast::FuncLiteral>(current_);

        auto func = parse_func_decl(false, sync);
        ret->func(func.take_node());
        if (!func)
            return error(std::move(ret));

        return ret;
    }

    // Array literal.
    case TokenType::LeftBracket: {
        auto lit = make_node<ast::ArrayLiteral>(current_);
        advance();

        static constexpr auto options = ListOptions(
            "array literal", TokenType::RightBracket)
                                            .set_allow_trailing_comma(true);

        const bool list_ok = parse_braced_list(
            options, sync, [&](TokenTypes inner_sync) {
                auto value = parse_expr(inner_sync);
                if (!value)
                    return false;

                lit->add_entry(value.take_node());
                return true;
            });

        return result(std::move(lit), list_ok);
    }

    // Map literal
    case TokenType::KwMap: {
        auto lit = make_node<ast::MapLiteral>(current_);
        advance();

        if (!expect(TokenType::LeftBrace))
            return error(std::move(lit));

        static constexpr auto options = ListOptions(
            "map literal", TokenType::RightBrace)
                                            .set_allow_trailing_comma(true);

        const bool list_ok = parse_braced_list(
            options, sync, [&](TokenTypes inner_sync) {
                auto key = parse_expr(inner_sync);
                if (!key)
                    return false;

                if (!expect(TokenType::Colon))
                    return false;

                auto value = parse_expr(inner_sync);
                if (!value)
                    return false;

                lit->add_entry(key.take_node(), value.take_node());
                return true;
            });

        return result(std::move(lit), list_ok);
    }

    // Set literal
    case TokenType::KwSet: {
        auto lit = make_node<ast::SetLiteral>(current_);
        advance();

        if (!expect(TokenType::LeftBrace))
            return error(std::move(lit));

        static constexpr auto options = ListOptions(
            "set literal", TokenType::RightBrace)
                                            .set_allow_trailing_comma(true);

        const bool list_ok = parse_braced_list(
            options, sync, [&](TokenTypes inner_sync) {
                auto value = parse_expr(inner_sync);
                if (!value)
                    return false;

                lit->add_entry(value.take_node());
                return true;
            });

        return result(std::move(lit), list_ok);
    }

    // Null Literal
    case TokenType::KwNull: {
        auto lit = make_node<ast::NullLiteral>(current_);
        lit->has_error(current_.has_error());
        advance();
        return lit;
    }

    // Boolean literals
    case TokenType::KwTrue:
    case TokenType::KwFalse: {
        auto lit = make_node<ast::BooleanLiteral>(
            current_, current_.type() == TokenType::KwTrue);
        advance();
        lit->has_error(current_.has_error());
        return lit;
    }

    // String literal
    case TokenType::StringLiteral: {
        auto str = make_node<ast::StringLiteral>(
            current_, current_.string_value());
        str->has_error(current_.has_error());
        advance();
        return str;
    }

    // Symbol literal
    case TokenType::SymbolLiteral: {
        auto sym = make_node<ast::SymbolLiteral>(
            current_, current_.string_value());
        sym->has_error(current_.has_error());
        advance();
        return sym;
    }

    // Integer literal
    case TokenType::IntegerLiteral: {
        auto lit = make_node<ast::IntegerLiteral>(
            current_, current_.int_value());
        lit->has_error(current_.has_error());
        advance();
        return lit;
    }

    // Float literal
    case TokenType::FloatLiteral: {
        auto lit = make_node<ast::FloatLiteral>(
            current_, current_.float_value());
        lit->has_error(current_.has_error());
        advance();
        return lit;
    }

    default:
        break;
    }

    diag_.reportf(Diagnostics::Error, current_.source(),
        "Unexpected {}, expected a valid expression.",
        to_description(current_.type()));
    return error();
}

Parser::Result<ast::BlockExpr> Parser::parse_block_expr(TokenTypes sync) {
    auto start_tok = expect(TokenType::LeftBrace);
    if (!start_tok)
        return error();

    auto block = make_node<ast::BlockExpr>(*start_tok);

    while (!accept(TokenType::RightBrace)) {
        if (current_.type() == TokenType::Eof) {
            diag_.reportf(Diagnostics::Error, current_.source(),
                "Unterminated block expression, expected {}.",
                to_description(TokenType::RightBrace));
            return error(std::move(block));
        }

        auto stmt = parse_stmt(sync.union_with(TokenType::RightBrace));
        stmt.with_node([&](auto&& node) { block->add_stmt(std::move(node)); });
        if (!stmt)
            goto recover;
    }

    return block;

recover:
    block->has_error(true);
    if (recover_consume(TokenType::RightBrace, sync))
        return block;
    return error(std::move(block));
}

Parser::Result<ast::IfExpr> Parser::parse_if_expr(TokenTypes sync) {
    auto start_tok = expect(TokenType::KwIf);
    if (!start_tok)
        return error();

    auto expr = make_node<ast::IfExpr>(*start_tok);

    {
        auto cond = parse_expr(TokenType::LeftBrace);
        expr->condition(cond.take_node());
        if (!cond && !recover_seek(TokenType::LeftBrace, sync))
            return error(std::move(expr));
    }

    {
        auto then_expr = parse_block_expr(sync.union_with(TokenType::KwElse));
        expr->then_branch(then_expr.take_node());
        if (!then_expr && !recover_seek(TokenType::KwElse, sync))
            return error(std::move(expr));
    }

    if (auto else_tok = accept(TokenType::KwElse)) {
        if (current_.type() == TokenType::KwIf) {
            auto nested = parse_if_expr(sync);
            expr->else_branch(nested.take_node());
            if (!nested)
                return error(std::move(expr));
        } else {
            auto else_expr = parse_block_expr(sync);
            expr->else_branch(else_expr.take_node());
            if (!else_expr)
                return error(std::move(expr));
        }
    }

    return expr;
}

Parser::Result<ast::Expr> Parser::parse_paren_expr(TokenTypes sync) {
    auto start_tok = expect(TokenType::LeftParen);
    if (!start_tok)
        return error();

    // "()" is the empty tuple.
    if (accept(TokenType::RightParen))
        return make_node<ast::TupleLiteral>(*start_tok);

    std::unique_ptr<ast::Expr> initial;

    // Parse the initial expression - don't know whether this is a tuple yet.
    {
        auto expr = parse_expr(
            sync.union_with({TokenType::Comma, TokenType::RightParen}));
        initial = expr.take_node();
        if (!expr)
            goto recover;

        auto next = expect({TokenType::Comma, TokenType::RightParen});
        if (!next)
            goto recover;

        // "(expr)" is not a tuple.
        if (next->type() == TokenType::RightParen)
            return initial;

        // "(expr, ..." is guaranteed to be a tuple.
        if (next->type() == TokenType::Comma)
            return parse_tuple(*start_tok, std::move(initial), sync);

        HAMMER_UNREACHABLE("Invalid token type.");
    }

recover:
    // Recover to either a ")" or a "," (whatever comes first).
    auto next = recover_consume(
        {TokenType::Comma, TokenType::RightParen}, sync);
    if (!next)
        return error(std::move(initial));

    // "( GARBAGE )"
    if (next->type() == TokenType::RightParen)
        return error(std::move(initial));

    // "( GARBAGE, ..."
    if (next->type() == TokenType::Comma)
        return parse_tuple(*start_tok, std::move(initial), sync);

    HAMMER_UNREACHABLE("Invalid token type.");
}

Parser::Result<ast::TupleLiteral> Parser::parse_tuple(const Token& start_tok,
    std::unique_ptr<ast::Expr> first_item, TokenTypes sync) {
    auto tuple = make_node<ast::TupleLiteral>(start_tok);
    if (first_item)
        tuple->add_entry(std::move(first_item));

    static constexpr auto options = ListOptions(
        "tuple literal", TokenType::RightParen)
                                        .set_allow_trailing_comma(true);

    const bool list_ok = parse_braced_list(
        options, sync, [&](TokenTypes inner_sync) {
            auto expr = parse_expr(inner_sync);
            if (expr.has_node())
                tuple->add_entry(expr.take_node());
            return expr.parse_ok();
        });

    return result(std::move(tuple), list_ok);
}

bool Parser::can_begin_var_decl(TokenType type) {
    return VAR_DECL_FIRST.contains(type);
}

bool Parser::can_begin_expression(TokenType type) {
    return EXPR_FIRST.contains(type);
}

SourceReference Parser::ref(size_t begin, size_t end) const {
    return SourceReference::from_std_offsets(file_name_, begin, end);
}

std::optional<Token> Parser::accept(TokenTypes tokens) {
    if (tokens.contains(current_.type())) {
        Token result = std::move(current_);
        advance();
        return {std::move(result)};
    }
    return {};
}

std::optional<Token> Parser::expect(TokenTypes tokens) {
    HAMMER_ASSERT(!tokens.empty(), "Token set must not be empty.");

    auto res = accept(tokens);
    if (!res) {
        diag_.report(Diagnostics::Error, current_.source(),
            unexpected_message({}, tokens, current_.type()));
    }
    return res;
}

bool Parser::recover_seek(TokenTypes expected, TokenTypes sync) {
    // TODO: It might be useful to track opening / closing braces in here?
    // We might be skipping over them otherwise.
    while (1) {
        if (current_.type() == TokenType::Eof)
            return false;

        if (expected.contains(current_.type()))
            return true;

        if (sync.contains(current_.type()))
            return false;

        advance();
    }
}

std::optional<Token>
Parser::recover_consume(TokenTypes expected, TokenTypes sync) {
    if (recover_seek(expected, sync)) {
        HAMMER_ASSERT(expected.contains(current_.type()), "Invalid token.");
        auto tok = std::move(current_);
        advance();
        return tok;
    }

    return {};
}

void Parser::advance() {
    current_ = lexer_.next();
}

} // namespace hammer
