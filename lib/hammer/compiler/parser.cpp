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

/*
 * TODO: Investigate whether scope guards (-> automatic recover with commit()) or 
 * try/catch blocks can improve the readability of the error handling logic in here.
 */

template<typename T, typename... Args>
constexpr bool one_of(const T& t, const Args&... args) {
    static_assert(sizeof...(Args) > 0,
        "Must have at least one argument to compare against.");
    return (... || (t == args));
}

template<typename Range>
bool contains(Range&& range, TokenType tok) {
    return std::find(range.begin(), range.end(), tok) != range.end();
}

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

static std::optional<ast::BinaryOperator> to_infix_operator(TokenType t) {
#define HAMMER_MAP_TOKEN(token, op) \
    case TokenType::token:          \
        return ast::BinaryOperator::op;

    switch (t) {
        HAMMER_MAP_TOKEN(Plus, Plus)
        HAMMER_MAP_TOKEN(Minus, Minus)
        HAMMER_MAP_TOKEN(Star, Multiply)
        HAMMER_MAP_TOKEN(Slash, Divide)
        HAMMER_MAP_TOKEN(Percent, Modulus)
        HAMMER_MAP_TOKEN(Starstar, Power)
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

static const TokenTypes EXPR_FIRST = {
    // Keywords
    TokenType::KwFunc,
    TokenType::KwContinue,
    TokenType::KwBreak,
    TokenType::KwReturn,
    TokenType::KwIf,
    TokenType::KwMap,

    // Literal constants
    TokenType::KwTrue,
    TokenType::KwFalse,
    TokenType::KwNull,

    // Literal values
    TokenType::Identifier,
    TokenType::StringLiteral,
    TokenType::FloatLiteral,
    TokenType::IntegerLiteral,

    // ( expr )
    TokenType::LeftParen,

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
bool Parser::parse_braced_list(std::string_view name, TokenType right_brace,
    bool allow_trailing_comma, TokenTypes sync, SubParser&& parser) {

    if (accept(right_brace))
        return true;

    const auto inner_sync = sync.union_with({TokenType::Comma, right_brace});

    while (1) {
        if (current_.type() == TokenType::Eof) {
            diag_.reportf(Diagnostics::Error, current_.source(),
                "Unterminated {}, expected {}.", name,
                to_description(right_brace));
            return false;
        }

        // Call the sub parser.
        const bool parser_ok = parser(inner_sync);

        // On success, we expect "," or closing brace.
        std::optional<Token> next;
        if (parser_ok)
            next = expect({TokenType::Comma, right_brace});

        // Either parser failed or expect failed
        if (!next) {
            if (!(next = recover_consume(
                      {TokenType::Comma, right_brace}, sync)))
                return false; // Recovery failed
        }

        if (next->type() == right_brace)
            return true;

        if (next->type() == TokenType::Comma) {
            // Trailing comma
            if (allow_trailing_comma && accept(right_brace))
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
    std::unique_ptr<ast::File> file = std::make_unique<ast::File>();
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
    case TokenType::Semicolon:
        advance();
        return std::make_unique<ast::EmptyStmt>();
    default:
        diag_.reportf(Diagnostics::Error, current_.source(), "Unexpected {}.",
            to_description(current_.type()));
        return error();
    }
}

Parser::Result<ast::ImportDecl> Parser::parse_import_decl(TokenTypes sync) {
    if (!expect(TokenType::KwImport))
        return error();

    auto decl = std::make_unique<ast::ImportDecl>();

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
    if (!expect(TokenType::KwFunc))
        return error();

    auto func = std::make_unique<ast::FuncDecl>();

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

    const bool list_ok = parse_braced_list("parameter list",
        TokenType::RightParen, false, sync,
        [&]([[maybe_unused]] TokenTypes inner_sync) {
            auto param_ident = expect(TokenType::Identifier);
            if (param_ident) {
                auto param = std::make_unique<ast::ParamDecl>();
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
    if (accept(TokenType::Semicolon))
        return std::make_unique<ast::EmptyStmt>();

    const TokenType type = current_.type();
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
        auto stmt = parse_var_decl(sync);
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

Parser::Result<ast::DeclStmt> Parser::parse_var_decl(TokenTypes sync) {
    const auto decl_tok = expect(VAR_DECL_FIRST);
    if (!decl_tok)
        return error();

    auto decl = std::make_unique<ast::DeclStmt>();

    auto ident = accept(TokenType::Identifier);
    if (!ident) {
        diag_.reportf(Diagnostics::Error, current_.source(),
            "Unexpected {}, expected a valid identifier.",
            to_description(current_.type()));
        return error(std::move(decl));
    }

    decl->declaration(std::make_unique<ast::VarDecl>());

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
    if (!expect(TokenType::KwWhile))
        return error();

    auto stmt = std::make_unique<ast::WhileStmt>();

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
    if (!expect(TokenType::KwFor))
        return error();

    auto stmt = std::make_unique<ast::ForStmt>();

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

    auto stmt = std::make_unique<ast::ExprStmt>();

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
    return parse_expr_precedence(0, sync);
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
Parser::parse_expr_precedence(int min_precedence, TokenTypes sync) {
    auto left = parse_prefix_expr(sync);
    if (!left)
        return left;

    while (1) {
        const auto op = to_infix_operator(current_.type());
        if (!op)
            break;

        const int op_precedence = ast::operator_precedence(*op);
        if (op_precedence < min_precedence)
            break;

        auto binary_expr = std::make_unique<ast::BinaryExpr>(*op);
        binary_expr->left_child(left.take_node());
        advance();

        const int next_precedence = ast::operator_is_right_associative(*op)
                                        ? op_precedence
                                        : op_precedence + 1;

        auto right = parse_expr_precedence(next_precedence, sync);
        binary_expr->right_child(right.take_node());
        if (!right)
            return error(std::move(binary_expr));

        left = std::move(binary_expr);
    }

    return left;
}

/*
 * Parses a unary expressions. Unary expressions are either plain primary
 * expressions or a unary operator followed by another unary expression.
 */
Parser::Result<ast::Expr> Parser::parse_prefix_expr(TokenTypes sync) {
    auto op = to_unary_operator(current_.type());
    if (!op) {
        auto expr = parse_primary_expr(sync);
        if (!expr)
            return expr;
        return parse_suffix_expr(expr.take_node(), sync);
    }

    // It's a unary operator
    auto unary = std::make_unique<ast::UnaryExpr>(*op);
    advance();

    auto inner = parse_prefix_expr(sync);
    unary->inner(inner.take_node());
    return forward(std::move(unary), inner);
}

/*
 * An expression may be followed by a suffix (like ".member" or "(args...)").
 */
Parser::Result<ast::Expr>
Parser::parse_suffix_expr(std::unique_ptr<ast::Expr> current, TokenTypes sync) {
    HAMMER_ASSERT_NOT_NULL(current);

    Result<ast::Expr> result(std::move(current));

    while (result.parse_ok()) {
        switch (current_.type()) {
        case TokenType::Dot:
            result = parse_dot_expr(result.take_node(), sync);
            break;
        case TokenType::LeftParen:
            result = parse_call_expr(result.take_node(), sync);
            break;
        case TokenType::LeftBracket:
            result = parse_index_expr(result.take_node(), sync);
            break;
        default:
            return result;
        }
    }

    return result;
}

Parser::Result<ast::DotExpr> Parser::parse_dot_expr(
    std::unique_ptr<ast::Expr> current, [[maybe_unused]] TokenTypes sync) {
    if (!expect(TokenType::Dot))
        return error();

    auto dot = std::make_unique<ast::DotExpr>();
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
    if (!expect(TokenType::LeftParen))
        return error();

    auto call = std::make_unique<ast::CallExpr>();
    call->func(std::move(current));

    const bool list_ok = parse_braced_list("argument list",
        TokenType::RightParen, false, sync, [&](TokenTypes inner_sync) {
            auto arg = parse_expr(inner_sync);
            arg.with_node([&](auto&& node) { call->add_arg(std::move(node)); });
            return arg.parse_ok();
        });

    return result(std::move(call), list_ok);
}

Parser::Result<ast::IndexExpr>
Parser::parse_index_expr(std::unique_ptr<ast::Expr> current, TokenTypes sync) {
    if (!expect(TokenType::LeftBracket))
        return error();

    auto expr = std::make_unique<ast::IndexExpr>();
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
        advance();

        auto ex = parse_expr(TokenType::RightParen);
        if (!expect(TokenType::RightParen))
            return error(ex.take_node());

        return ex;
    }

    // If expression
    case TokenType::KwIf: {
        return parse_if_expr(sync);
    }

    // Return expression
    case TokenType::KwReturn: {
        auto ret = std::make_unique<ast::ReturnExpr>();
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
    case TokenType::KwContinue:
        advance();
        return std::make_unique<ast::ContinueExpr>();

    // Break expression
    case TokenType::KwBreak:
        advance();
        return std::make_unique<ast::BreakExpr>();

    // Variable reference
    case TokenType::Identifier: {
        const bool has_error = current_.has_error();
        auto id = std::make_unique<ast::VarExpr>(current_.string_value());
        advance();
        return result(std::move(id), !has_error);
    }

    // Function Literal
    case TokenType::KwFunc: {
        auto ret = std::make_unique<ast::FuncLiteral>();
        advance();

        auto func = parse_func_decl(false, sync);
        ret->func(func.take_node());
        if (!func)
            return error(std::move(ret));

        return ret;
    }

    // Map literal
    case TokenType::KwMap: {
        auto lit = std::make_unique<ast::MapLiteral>();
        advance();

        if (!expect(TokenType::LeftBrace))
            return error(std::move(lit));

        const bool list_ok = parse_braced_list("map literal",
            TokenType::RightBrace, true, sync, [&](TokenTypes inner_sync) {
                auto key_token = expect(TokenType::StringLiteral);
                if (!key_token || key_token->has_error())
                    return false;

                if (!expect(TokenType::Colon))
                    return false;

                auto expr = parse_expr(inner_sync);
                if (!lit->add_entry(
                        key_token->string_value(), expr.take_node())) {
                    lit->has_error(true);
                    diag_.reportf(Diagnostics::Error, key_token->source(),
                        "Duplicate key in map literal.");
                    // Continue parsing
                }

                return expr.parse_ok();
            });

        return result(std::move(lit), list_ok);
    }

    // Null Literal
    case TokenType::KwNull: {
        auto lit = std::make_unique<ast::NullLiteral>();
        lit->has_error(current_.has_error());
        advance();
        return lit;
    }

    // Boolean literals
    case TokenType::KwTrue:
    case TokenType::KwFalse: {
        auto lit = std::make_unique<ast::BooleanLiteral>(
            current_.type() == TokenType::KwTrue);
        advance();
        lit->has_error(current_.has_error());
        return lit;
    }

    // String literal
    case TokenType::StringLiteral: {
        auto str = std::make_unique<ast::StringLiteral>(
            current_.string_value());
        str->has_error(current_.has_error());
        advance();
        return str;
    }

    // Integer literal
    case TokenType::IntegerLiteral: {
        auto lit = std::make_unique<ast::IntegerLiteral>(current_.int_value());
        lit->has_error(current_.has_error());
        advance();
        return lit;
    }

    // Float literal
    case TokenType::FloatLiteral: {
        auto lit = std::make_unique<ast::FloatLiteral>(current_.float_value());
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
    if (!expect(TokenType::LeftBrace))
        return error();

    auto block = std::make_unique<ast::BlockExpr>();

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
    if (!expect(TokenType::KwIf))
        return error();

    auto expr = std::make_unique<ast::IfExpr>();

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
