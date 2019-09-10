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
    case TokenType::LNot:
        return ast::UnaryOperator::LogicalNot;
    case TokenType::BNot:
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

        // TODO left / right shift

        HAMMER_MAP_TOKEN(BAnd, BitwiseAnd)
        HAMMER_MAP_TOKEN(BOr, BitwiseOr)
        HAMMER_MAP_TOKEN(BXor, BitwiseXor)

        HAMMER_MAP_TOKEN(Less, Less)
        HAMMER_MAP_TOKEN(LessEq, LessEq)
        HAMMER_MAP_TOKEN(Greater, Greater)
        HAMMER_MAP_TOKEN(GreaterEq, GreaterEq)
        HAMMER_MAP_TOKEN(EqEq, Equals)
        HAMMER_MAP_TOKEN(NEq, NotEquals)
        HAMMER_MAP_TOKEN(LAnd, LogicalAnd)
        HAMMER_MAP_TOKEN(LOr, LogicalOr)

        HAMMER_MAP_TOKEN(Eq, Assign)

    default:
        return {};
    }

#undef HAMMER_MAP_TOKEN
}

static std::string unexpected_message(
    std::string_view context, TokenTypes expected, TokenType seen) {
    HAMMER_ASSERT(expected.size() > 0, "Invalid expected set.");

    const size_t size = expected.size();

    fmt::memory_buffer buf;
    if (!context.empty()) {
        fmt::format_to(
            buf, "Unexpected {} in {} context", to_description(seen), context);
    } else {
        fmt::format_to(buf, "Unexpected {}", to_description(seen));
    }

    if (size <= 3) {
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
    TokenType::LParen,

    // { statements ... }
    TokenType::LBrace,

    // Unary operators
    TokenType::Plus,
    TokenType::Minus,
    TokenType::BNot,
    TokenType::LNot,
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
    TokenType::LBrace,
};

Parser::Parser(std::string_view file_name, std::string_view source,
    StringTable& strings, Diagnostics& diag)
    : file_name_(strings.insert(file_name))
    , source_(source)
    , strings_(strings)
    , diag_(diag)
    , lexer_(file_name_, source_, strings_, diag_) {

    advance();
}

template<typename Operation>
auto Parser::check_error(std::string_view context, TokenTypes first,
    TokenTypes follow, Operation&& op) {
    decltype(op()) node;

    if (!first.contains(current_.type())) {
        diag_.report(Diagnostics::Error, current_.source(),
            unexpected_message(context, first, current_.type()));

        while (1) {
            advance();

            if (first.contains(current_.type()))
                break;

            if (current_.type() == TokenType::Eof
                || follow.contains(current_.type()))
                return node;
        }
    }

    bool first_error = true;

again:
    node = op();

    while (current_.type() != TokenType::Eof
           && !follow.contains(current_.type())) {
        if (!node && first.contains(current_.type()))
            goto again;

        if (node && !node->has_error() && first_error) {
            first_error = false;
            node->has_error(true);
            diag_.report(Diagnostics::Error, current_.source(),
                unexpected_message(context, follow, current_.type()));
        }

        advance();
    }
    return node;
}

std::unique_ptr<ast::File> Parser::parse_file() {
    std::unique_ptr<ast::File> file = std::make_unique<ast::File>();
    file->file_name(file_name_);

    // TODO: possibly handle unbalanced braces in here
    while (!accept(TokenType::Eof)) {
        if (auto item = parse_toplevel_item(TOPLEVEL_ITEM_FIRST)) {
            file->add_item(std::move(item));
        } else {
            file->has_error(true);
            break;
        }
    }

    return file;
}

std::unique_ptr<ast::Node> Parser::parse_toplevel_item(TokenTypes follow) {
    return check_error("toplevel item", TOPLEVEL_ITEM_FIRST, follow,
        [&]() -> std::unique_ptr<ast::Node> {
            switch (current_.type()) {
            case TokenType::KwImport:
                return parse_import_decl();
            case TokenType::KwFunc:
                return parse_func_decl(true, follow);
            case TokenType::Semicolon:
                advance();
                return std::make_unique<ast::EmptyStmt>();
            default:
                diag_.reportf(Diagnostics::Error, current_.source(),
                    "Unexpected {}.", to_description(current_.type()));
                return nullptr;
            }
        });
}

std::unique_ptr<ast::ImportDecl> Parser::parse_import_decl() {
    if (!expect(TokenType::KwImport))
        return nullptr;

    auto decl = std::make_unique<ast::ImportDecl>();

    auto ident = expect(TokenType::Identifier);
    if (!ident) {
        decl->has_error(true);
        return decl;
    }

    decl->name(ident->string_value());
    if (ident->has_error()) {
        decl->has_error(true);
        return decl;
    }

    if (!expect(TokenType::Semicolon)) {
        decl->has_error(true);
        return decl;
    }

    return decl;
}

std::unique_ptr<ast::FuncDecl> Parser::parse_func_decl(
    bool requires_name, TokenTypes follow) {
    if (!expect(TokenType::KwFunc))
        return nullptr;

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

    if (!expect(TokenType::LParen)) {
        func->has_error(true);
        return func;
    }

    if (!accept(TokenType::RParen)) {
        while (1) {
            if (auto param_ident = expect(TokenType::Identifier)) {
                auto param = std::make_unique<ast::ParamDecl>();
                param->name(param_ident->string_value());
                func->add_param(std::move(param));
            }

            if (accept(TokenType::RParen))
                break;

            if (accept(TokenType::Comma))
                continue;

            diag_.reportf(Diagnostics::Error, current_.source(),
                "Expected {} or {} in function parameter list but saw a {} "
                "instead.",
                to_description(TokenType::Comma),
                to_description(TokenType::RParen),
                to_description(current_.type()));
            func->has_error(true);
            return nullptr;
        }
    }

    if (auto body = parse_block_expr(follow)) {
        func->body(std::move(body));
    } else {
        func->has_error(true);
    }
    return func;
}

std::unique_ptr<ast::Stmt> Parser::parse_stmt(TokenTypes follow) {
    return check_error(
        "statement", STMT_FIRST, follow, [&]() -> std::unique_ptr<ast::Stmt> {
            if (accept(TokenType::Semicolon))
                return std::make_unique<ast::EmptyStmt>();

            const TokenType type = current_.type();

            if (type == TokenType::KwWhile) {
                auto stmt = parse_while_stmt(follow);
                accept(TokenType::Semicolon);
                return stmt;
            }

            if (type == TokenType::KwFor) {
                auto stmt = parse_for_stmt(follow);
                accept(TokenType::Semicolon);
                return stmt;
            }

            if (can_begin_var_decl(type)) {
                auto stmt = parse_var_decl(TokenType::Semicolon);
                expect(TokenType::Semicolon);
                return stmt;
            }

            if (can_begin_expression(type)) {
                return parse_expr_stmt(follow);
            }

            // Hint: can_begin_expression could be out of sync with
            // the expression parser.
            diag_.reportf(Diagnostics::Error, current_.source(),
                "Unexpected {} in statement context.", to_description(type));
            return nullptr;
        });
}

std::unique_ptr<ast::DeclStmt> Parser::parse_var_decl(TokenTypes follow) {
    return check_error("declaration", VAR_DECL_FIRST, follow,
        [&]() -> std::unique_ptr<ast::DeclStmt> {
            const auto decl_tok = expect(VAR_DECL_FIRST);
            if (!decl_tok)
                return nullptr;

            auto decl = std::make_unique<ast::DeclStmt>();

            auto ident = accept(TokenType::Identifier);
            if (!ident) {
                diag_.reportf(Diagnostics::Error, current_.source(),
                    "Unexpected {}, expected a valid identifier.",
                    to_description(current_.type()));
                decl->has_error(true);
                return decl;
            }

            decl->declaration(std::make_unique<ast::VarDecl>());

            ast::VarDecl* var = decl->declaration();
            var->is_const(decl_tok->type() == TokenType::KwConst);
            var->name(ident->string_value());

            if (ident->has_error()) {
                var->has_error(true);
            }

            if (!accept(TokenType::Eq)) {
                return decl;
            }

            if (auto expr = parse_expr(follow)) {
                var->initializer(std::move(expr));
            } else {
                var->has_error(true);
            }
            return decl;
        });
}

std::unique_ptr<ast::WhileStmt> Parser::parse_while_stmt(TokenTypes follow) {
    if (!expect(TokenType::KwWhile))
        return nullptr;

    auto stmt = std::make_unique<ast::WhileStmt>();

    if (auto cond_expr = parse_expr({TokenType::LBrace})) {
        stmt->condition(std::move(cond_expr));
    } else {
        stmt->has_error(true);
        return stmt;
    }

    if (auto body = parse_block_expr(follow)) {
        stmt->body(std::move(body));
    } else {
        stmt->has_error(true);
    }

    return stmt;
}

std::unique_ptr<ast::ForStmt> Parser::parse_for_stmt(TokenTypes follow) {
    if (!expect(TokenType::KwFor))
        return nullptr;

    auto stmt = std::make_unique<ast::ForStmt>();

    // A leading ( is optional
    const bool optional_paren = static_cast<bool>(accept(TokenType::LParen));

    // Optional var decl
    if (!accept(TokenType::Semicolon)) {
        if (!can_begin_var_decl(current_.type())) {
            diag_.reportf(Diagnostics::Error, current_.source(),
                "Expected a variable declaration or a {}.",
                to_description(TokenType::Semicolon));
            stmt->has_error(true);
        } else {
            if (auto decl = parse_var_decl({TokenType::Semicolon})) {
                stmt->decl(std::move(decl));
            } else {
                stmt->has_error(true);
            }
        }

        if (!expect(TokenType::Semicolon)) {
            stmt->has_error(true);
            return stmt;
        }
    }

    // Optional loop condition
    if (!accept(TokenType::Semicolon)) {
        if (auto expr = parse_expr({TokenType::Semicolon})) {
            stmt->condition(std::move(expr));
        } else {
            stmt->has_error(true);
        }

        if (!expect(TokenType::Semicolon)) {
            stmt->has_error(true);
            return stmt;
        }
    }

    // Optional step expression
    if (optional_paren ? current_.type() != TokenType::RParen
                       : current_.type() != TokenType::LBrace) {
        if (auto expr = parse_expr(
                optional_paren ? TokenType::RParen : TokenType::LBrace)) {
            stmt->step(std::move(expr));
        } else {
            stmt->has_error(true);
        }
    }

    // The closing ) if a ( was seen.
    if (optional_paren && !expect(TokenType::RParen)) {
        stmt->has_error(true);
        return stmt;
    }

    // Loop body
    if (auto block = parse_block_expr(follow)) {
        stmt->body(std::move(block));
    } else {
        stmt->has_error(true);
    }

    return stmt;
}

std::unique_ptr<ast::ExprStmt> Parser::parse_expr_stmt(TokenTypes follow) {
    const bool need_semicolon = !EXPR_STMT_OPTIONAL_SEMICOLON.contains(
        current_.type());

    auto stmt = std::make_unique<ast::ExprStmt>();
    if (auto expr = parse_expr(follow.union_with(TokenType::Semicolon))) {
        stmt->expression(std::move(expr));
    } else {
        stmt->has_error(true);
        return stmt;
    }

    if (need_semicolon) {
        if (!expect(TokenType::Semicolon))
            stmt->has_error(true);
    } else {
        accept(TokenType::Semicolon);
    }
    return stmt;
}

std::unique_ptr<ast::Expr> Parser::parse_expr(TokenTypes follow) {
    return check_error("expression", EXPR_FIRST, follow,
        [&] { return parse_expr_precedence(0, follow); });
}

/*
 * Recursive function that implements a pratt parser.
 *
 * See also:
 *      http://crockford.com/javascript/tdop/tdop.html
 *      https://www.oilshell.org/blog/2016/11/01.html
 *      https://groups.google.com/forum/#!topic/comp.compilers/ruJLlQTVJ8o
 */
std::unique_ptr<ast::Expr> Parser::parse_expr_precedence(
    int min_precedence, TokenTypes follow) {
    auto left = parse_prefix_expr(follow);

    while (1) {
        const auto op = to_infix_operator(current_.type());
        if (!op)
            break;

        const int op_precedence = ast::operator_precedence(*op);
        if (op_precedence < min_precedence)
            break;

        // TODO implement ternary condition operator here.
        auto binary_expr = std::make_unique<ast::BinaryExpr>(*op);
        binary_expr->left_child(std::move(left));
        advance();

        const int next_precedence = ast::operator_is_right_associative(*op)
                                        ? op_precedence
                                        : op_precedence + 1;
        binary_expr->right_child(
            parse_expr_precedence(next_precedence, follow));

        left = std::move(binary_expr);
    }

    return left;
}

/*
 * Parses a unary expressions. Unary expressions are either plain primary
 * expressions or a unary operator followed by another unary expression.
 */
std::unique_ptr<ast::Expr> Parser::parse_prefix_expr(TokenTypes follow) {
    auto op = to_unary_operator(current_.type());
    if (!op) {
        return parse_suffix_expr(follow);
    }

    // It's a unary operator
    auto node = std::make_unique<ast::UnaryExpr>(*op);
    advance();

    if (auto expr = parse_prefix_expr(follow)) {
        node->inner(std::move(expr));
    }
    return node;
}

std::unique_ptr<ast::Expr> Parser::parse_suffix_expr(TokenTypes follow) {
    auto expr = parse_primary_expr(follow);
    if (!expr) {
        return nullptr;
    }

    return parse_suffix_expr_inner(std::move(expr));
}

std::unique_ptr<ast::Expr> Parser::parse_suffix_expr_inner(
    std::unique_ptr<ast::Expr> current) {
    HAMMER_ASSERT_NOT_NULL(current);

    // Dot expr
    if (accept(TokenType::Dot)) {
        auto dot = std::make_unique<ast::DotExpr>();
        dot->inner(std::move(current));

        if (auto ident_tok = expect(TokenType::Identifier)) {
            dot->name(ident_tok->string_value());
            if (ident_tok->has_error()) {
                dot->has_error(true);
            }
        } else {
            dot->has_error(true);
        }

        return parse_suffix_expr_inner(std::move(dot));
    }

    // Call expr
    if (accept(TokenType::LParen)) {
        auto call = std::make_unique<ast::CallExpr>();
        call->func(std::move(current));

        if (!accept(TokenType::RParen)) {
            while (1) {
                if (auto arg = parse_expr(
                        {TokenType::RParen, TokenType::Comma})) {
                    call->add_arg(std::move(arg));
                }

                if (accept(TokenType::RParen))
                    break;

                if (accept(TokenType::Comma))
                    continue;

                diag_.reportf(Diagnostics::Error, current_.source(),
                    "Expected {} or {} in function argument list but "
                    "encountered a {} instead.",
                    to_description(TokenType::Comma),
                    to_description(TokenType::RParen),
                    to_description(current_.type()));
                call->has_error(true);
                return call;
            }
        }

        return parse_suffix_expr_inner(std::move(call));
    }

    // Index expr
    if (accept(TokenType::LBracket)) {
        auto expr = std::make_unique<ast::IndexExpr>();
        expr->inner(std::move(current));

        if (auto arg = parse_expr({TokenType::RBracket})) {
            expr->index(std::move(arg));
        }

        if (!expect(TokenType::RBracket)) {
            expr->has_error(true);
            return expr;
        }

        return parse_suffix_expr_inner(std::move(expr));
    }

    return current;
}

std::unique_ptr<ast::Expr> Parser::parse_primary_expr(TokenTypes follow) {
    switch (current_.type()) {

    // Block expr
    case TokenType::LBrace: {
        return parse_block_expr(follow);
    }

    // Braced subexpression
    case TokenType::LParen: {
        advance();

        auto ex = parse_expr({TokenType::RParen});
        if (!expect(TokenType::RParen) && ex)
            ex->has_error(true);
        return ex;
    }

    // If expression
    case TokenType::KwIf: {
        return parse_if_expr(follow);
    }

    // Return expression
    case TokenType::KwReturn: {
        auto ret = std::make_unique<ast::ReturnExpr>();
        advance();

        if (can_begin_expression(current_.type())) {
            if (auto expr = parse_expr(follow)) {
                ret->inner(std::move(expr));
            }
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
        auto id = std::make_unique<ast::VarExpr>(current_.string_value());
        id->has_error(current_.has_error());
        advance();
        return id;
    }

    // Function Literal
    case TokenType::KwFunc: {
        auto ret = std::make_unique<ast::FuncLiteral>();
        advance();

        auto func = parse_func_decl(false, follow);
        ret->func(std::move(func));
        return ret;
    }

    // Map literal
    case TokenType::KwMap: {
        auto lit = std::make_unique<ast::MapLiteral>();
        advance();

        if (!expect(TokenType::LBrace)) {
            lit->has_error(true);
            return lit;
        }

        if (accept(TokenType::RBrace))
            return lit;

        while (1) {
            if (current_.type() == TokenType::Eof) {
                diag_.reportf(Diagnostics::Error, current_.source(),
                    "Unterminated map literal.");
                break;
            }

            auto key_token = expect(TokenType::StringLiteral);
            if (!key_token || key_token->has_error()) {
                lit->has_error(true);
                break;
            }

            if (!expect(TokenType::Colon)) {
                lit->has_error(true);
                break;
            }

            auto expr = parse_expr({TokenType::RBrace, TokenType::Comma});
            if (!expr) {
                lit->has_error(true);
                break;
            }

            if (!lit->add_entry(key_token->string_value(), std::move(expr))) {
                diag_.reportf(Diagnostics::Error, key_token->source(),
                    "Duplicate key in map literal.");
                lit->has_error(true);
                // Continue parsing
            }

            if (accept(TokenType::Comma))
                continue;

            if (accept(TokenType::RBrace))
                break;

            diag_.report(Diagnostics::Error, current_.source(),
                unexpected_message("map literal",
                    {TokenType::Comma, TokenType::RBrace}, current_.type()));
            lit->has_error(true);
            break;
        }

        return lit;
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
    return nullptr;
}

std::unique_ptr<ast::BlockExpr> Parser::parse_block_expr(TokenTypes follow) {
    return check_error("block", TokenType::LBrace, follow,
        [&]() -> std::unique_ptr<ast::BlockExpr> {
            if (!expect(TokenType::LBrace))
                return nullptr;

            std::unique_ptr<ast::BlockExpr> block =
                std::make_unique<ast::BlockExpr>();
            while (!accept(TokenType::RBrace)) {
                if (current_.type() == TokenType::Eof) {
                    diag_.reportf(Diagnostics::Error, current_.source(),
                        "Unterminated block expression.");
                    break;
                }

                if (auto stmt = parse_stmt(
                        STMT_FIRST.union_with(TokenType::RBrace))) {
                    block->add_stmt(std::move(stmt));
                } else {
                    block->has_error(true);
                    break;
                }
            }

            return block;
        });
}

std::unique_ptr<ast::IfExpr> Parser::parse_if_expr(TokenTypes follow) {
    if (!expect(TokenType::KwIf))
        return nullptr;

    auto expr = std::make_unique<ast::IfExpr>();

    if (auto cond_expr = parse_expr({TokenType::LBrace})) {
        expr->condition(std::move(cond_expr));
    } else {
        expr->has_error(true);
    }

    if (auto then_expr = parse_block_expr(
            follow.union_with(TokenType::KwElse))) {
        expr->then_branch(std::move(then_expr));
    } else {
        expr->has_error(true);
    }

    if (auto else_tok = accept(TokenType::KwElse)) {
        if (current_.type() == TokenType::KwIf) {
            expr->else_branch(parse_if_expr(follow));
        } else {
            if (auto else_expr = parse_block_expr(follow)) {
                expr->else_branch(std::move(else_expr));
            } else {
                expr->has_error(true);
            }
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
    HAMMER_ASSERT(tokens.size() > 0, "Token set must not be empty.");

    auto res = accept(tokens);
    if (!res) {
        diag_.report(Diagnostics::Error, current_.source(),
            unexpected_message({}, tokens, current_.type()));
    }
    return res;
}

void Parser::advance() {
    current_ = lexer_.next();
}

} // namespace hammer
