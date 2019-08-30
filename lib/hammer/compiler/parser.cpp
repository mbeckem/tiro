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
    static_assert(sizeof...(Args) > 0, "Must have at least one argument to compare against.");
    return (... || (t == args));
}

template<typename Range>
bool contains(Range&& range, TokenType tok) {
    return std::find(range.begin(), range.end(), tok) != range.end();
}

static std::optional<ast::UnaryOperator> to_unary_operator(TokenType t) {
    switch (t) {
    case TokenType::plus:
        return ast::UnaryOperator::plus;
    case TokenType::minus:
        return ast::UnaryOperator::minus;
    case TokenType::lnot:
        return ast::UnaryOperator::logical_not;
    case TokenType::bnot:
        return ast::UnaryOperator::bitwise_not;
    default:
        return {};
    }
}

static std::optional<ast::BinaryOperator> to_infix_operator(TokenType t) {
#define HAMMER_MAP_TOKEN(token, op) \
    case TokenType::token:          \
        return ast::BinaryOperator::op;

    switch (t) {
        HAMMER_MAP_TOKEN(plus, plus)
        HAMMER_MAP_TOKEN(minus, minus)
        HAMMER_MAP_TOKEN(star, multiply)
        HAMMER_MAP_TOKEN(slash, divide)
        HAMMER_MAP_TOKEN(percent, modulus)
        HAMMER_MAP_TOKEN(starstar, power)

        // TODO left / right shift

        HAMMER_MAP_TOKEN(band, bitwise_and)
        HAMMER_MAP_TOKEN(bor, bitwise_or)
        HAMMER_MAP_TOKEN(bxor, bitwise_xor)

        HAMMER_MAP_TOKEN(less, less)
        HAMMER_MAP_TOKEN(lesseq, less_eq)
        HAMMER_MAP_TOKEN(greater, greater)
        HAMMER_MAP_TOKEN(greatereq, greater_eq)
        HAMMER_MAP_TOKEN(eqeq, equals)
        HAMMER_MAP_TOKEN(neq, not_equals)
        HAMMER_MAP_TOKEN(land, logical_and)
        HAMMER_MAP_TOKEN(lor, logical_or)

        HAMMER_MAP_TOKEN(eq, assign)

    default:
        return {};
    }

#undef HAMMER_MAP_TOKEN
}

Parser::Parser(std::string_view file_name, std::string_view source, StringTable& strings,
               Diagnostics& diag)
    : file_name_(strings.insert(file_name))
    , source_(source)
    , strings_(strings)
    , diag_(diag)
    , lexer_(file_name_, source_, strings_, diag_) {

    advance();
}

template<typename Operation>
auto Parser::check_error(Operation&& op) {
    decltype(op.parse()) node;

    if (!op.first(current_.type())) {
        diag_.reportf(Diagnostics::error, current_.source(), "Unexpected {}, expected {}.",
                      to_helpful_string(current_.type()), op.expected());

        while (1) {
            advance();

            if (op.first(current_.type()))
                break;

            if (current_.type() == TokenType::eof || op.follow(current_.type()))
                return node;
        }
    }

again:
    node = op.parse();

    if (!node || node->has_error()) {
        if (current_.type() == TokenType::eof || op.follow(current_.type()))
            return node;

        if (!node && op.first(current_.type()))
            goto again;

        advance();
    }

    return node;
}

std::unique_ptr<ast::File> Parser::parse_file() {
    std::unique_ptr<ast::File> file = std::make_unique<ast::File>();
    file->file_name(file_name_);

    while (1) {
        if (accept(TokenType::eof))
            break;

        if (current_.type() == TokenType::kw_import) {
            file->add_item(parse_import_decl());
            continue;
        }

        if (current_.type() == TokenType::kw_func) {
            file->add_item(parse_func_decl(true));
            continue;
        }

        if (TokenType t = current_.type();
            one_of(t, TokenType::rparen, TokenType::rbracket, TokenType::rbrace)) {
            diag_.reportf(Diagnostics::error, current_.source(), "Unbalanced {} at the top level.",
                          to_helpful_string(t));
            file->has_error(true);
            break;
        }

        diag_.reportf(Diagnostics::error, current_.source(),
                      "Expected a valid top-level item but saw a {} instead.",
                      to_helpful_string(current_.type()));
        file->has_error(true);
        break;
    }

    return file;
}

std::unique_ptr<ast::ImportDecl> Parser::parse_import_decl() {
    if (!expect(TokenType::kw_import))
        return nullptr;

    auto decl = std::make_unique<ast::ImportDecl>();

    auto ident = expect(TokenType::identifier);
    if (!ident) {
        decl->has_error(true);
    }

    decl->name(ident->string_value());
    if (ident->has_error()) {
        decl->has_error(true);
    }
    // TODO: More syntax

    return decl;
}

std::unique_ptr<ast::FuncDecl> Parser::parse_func_decl(bool requires_name) {
    if (!expect(TokenType::kw_func))
        return nullptr;

    auto func = std::make_unique<ast::FuncDecl>();

    if (auto ident = accept(TokenType::identifier)) {
        func->name(ident->string_value());
        if (ident->has_error()) {
            func->has_error(true);
        }
    } else if (requires_name) {
        diag_.reportf(
            Diagnostics::error, current_.source(),
            "Expected a valid identifier for the new function's name but saw a {} instead.",
            to_helpful_string(current_.type()));
        func->has_error(true);
    }

    if (!expect(TokenType::lparen)) {
        func->has_error(true);
        return func;
    }

    if (!accept(TokenType::rparen)) {
        while (1) {
            if (auto param_ident = expect(TokenType::identifier)) {
                auto param = std::make_unique<ast::ParamDecl>();
                param->name(param_ident->string_value());
                func->add_param(std::move(param));
            }

            if (accept(TokenType::rparen))
                break;

            if (accept(TokenType::comma))
                continue;

            diag_.reportf(Diagnostics::error, current_.source(),
                          "Expected {} or {} in function parameter list but saw a {} instead.",
                          to_helpful_string(TokenType::comma), to_helpful_string(TokenType::rparen),
                          to_helpful_string(current_.type()));
            func->has_error(true);
            return nullptr;
        }
    }

    if (auto body = parse_block_expr()) {
        func->body(std::move(body));
    } else {
        func->has_error(true);
    }
    return func;
}

std::unique_ptr<ast::Stmt> Parser::parse_stmt() {
    struct Operation {
        Parser* parser_;
        // TODO follow

        bool first(TokenType type) {
            return type == TokenType::semicolon || can_begin_var_decl(type)
                   || type == TokenType::kw_while || type == TokenType::kw_for
                   || can_begin_expression(type);
        }

        bool follow(TokenType type) {
            unused(type);
            return false;
        }

        std::unique_ptr<ast::Stmt> parse() {
            if (parser_->accept(TokenType::semicolon))
                return std::make_unique<ast::EmptyStmt>();

            const TokenType type = parser_->current_.type();

            if (can_begin_var_decl(type)) {
                auto stmt = parser_->parse_var_decl(TokenType::semicolon);
                parser_->expect(TokenType::semicolon);
                return stmt;
            }

            if (type == TokenType::kw_while) {
                auto stmt = parser_->parse_while_stmt();
                parser_->accept(TokenType::semicolon);
                return stmt;
            }

            if (type == TokenType::kw_for) {
                auto stmt = parser_->parse_for_stmt();
                parser_->accept(TokenType::semicolon);
                return stmt;
            }

            if (can_begin_expression(type)) {
                return parser_->parse_expr_stmt();
            }

            // Hint: can_begin_expression could be out of sync with the expression parser.
            parser_->diag_.reportf(Diagnostics::error, parser_->current_.source(),
                                   "Unexpected {} in statement context.", to_helpful_string(type));
            return nullptr;
        }

        std::string_view expected() { return "a statement"; }
    };

    return check_error(Operation{this});
}

std::unique_ptr<ast::DeclStmt> Parser::parse_var_decl(Span<const TokenType> follow) {
    const TokenType type = current_.type();
    if (!one_of(type, TokenType::kw_var, TokenType::kw_const)) {
        diag_.report(Diagnostics::error, current_.source(),
                     "Expected a variable declaration in this context.");
        return nullptr;
    }

    auto decl = std::make_unique<ast::DeclStmt>();
    advance();

    auto ident = accept(TokenType::identifier);
    if (!ident) {
        diag_.reportf(Diagnostics::error, current_.source(),
                      "Expected a valid identifier for the new variable's name but saw a {}.",
                      to_helpful_string(current_.type()));
        decl->has_error(true);
        return decl;
    }

    decl->declaration(std::make_unique<ast::VarDecl>());

    ast::VarDecl* var = decl->declaration();
    var->is_const(type == TokenType::kw_const);
    var->name(ident->string_value());

    if (ident->has_error()) {
        var->has_error(true);
    }

    if (!accept(TokenType::eq)) {
        return decl;
    }

    if (auto expr = parse_expr(follow)) {
        var->initializer(std::move(expr));
    }

    return decl;
}

std::unique_ptr<ast::WhileStmt> Parser::parse_while_stmt() {
    if (!expect(TokenType::kw_while))
        return nullptr;

    auto stmt = std::make_unique<ast::WhileStmt>();

    if (auto cond_expr = parse_expr({TokenType::lbrace})) {
        stmt->condition(std::move(cond_expr));
    } else {
        stmt->has_error(true);
        return stmt;
    }

    if (auto body = parse_block_expr()) {
        stmt->body(std::move(body));
    } else {
        stmt->has_error(true);
    }

    return stmt;
}

std::unique_ptr<ast::ForStmt> Parser::parse_for_stmt() {
    if (!expect(TokenType::kw_for))
        return nullptr;

    auto stmt = std::make_unique<ast::ForStmt>();

    // A leading ( is optional
    const bool optional_paren = static_cast<bool>(accept(TokenType::lparen));

    // Optional var decl
    if (!accept(TokenType::semicolon)) {
        if (!can_begin_var_decl(current_.type())) {
            diag_.reportf(Diagnostics::error, current_.source(),
                          "Expected a variable declaration or a {}.",
                          to_helpful_string(TokenType::semicolon));
            stmt->has_error(true);
        } else {
            if (auto decl = parse_var_decl({TokenType::semicolon})) {
                stmt->decl(std::move(decl));
            } else {
                stmt->has_error(true);
            }
        }

        if (!expect(TokenType::semicolon)) {
            stmt->has_error(true);
            return stmt;
        }
    }

    // Optional loop condition
    if (!accept(TokenType::semicolon)) {
        if (auto expr = parse_expr({TokenType::semicolon})) {
            stmt->condition(std::move(expr));
        } else {
            stmt->has_error(true);
        }

        if (!expect(TokenType::semicolon)) {
            stmt->has_error(true);
            return stmt;
        }
    }

    // Optional step expression
    if (optional_paren ? current_.type() != TokenType::rparen
                       : current_.type() != TokenType::lbrace) {
        if (auto expr = parse_expr(optional_paren ? TokenType::rparen : TokenType::lbrace)) {
            stmt->step(std::move(expr));
        } else {
            stmt->has_error(true);
        }
    }

    // The closing ) if a ( was seen.
    if (optional_paren && !expect(TokenType::rparen)) {
        stmt->has_error(true);
        return stmt;
    }

    // TODO continue on error for diagnostics

    // Loop body
    if (auto block = parse_block_expr()) { // FIXME
        stmt->body(std::move(block));
    } else {
        stmt->has_error(true);
    }

    return stmt;
}

std::unique_ptr<ast::ExprStmt> Parser::parse_expr_stmt() {
    if (auto expr = parse_expr({TokenType::semicolon})) {
        auto stmt = std::make_unique<ast::ExprStmt>();

        stmt->expression(std::move(expr));
        if (stmt->expression()->has_error()) {
            stmt->has_error(true);
            return stmt;
        }

        if (isa<ast::FuncLiteral>(stmt->expression()) || isa<ast::BlockExpr>(stmt->expression())
            || isa<ast::IfExpr>(stmt->expression())) {
            // Semicolon optional
            accept(TokenType::semicolon);
        } else {
            if (!expect(TokenType::semicolon)) {
                stmt->has_error(true);
            }
        }

        return stmt;
    }
    return nullptr;
}

std::unique_ptr<ast::Expr> Parser::parse_expr(Span<const TokenType> follow) {
    struct Operation {
        Parser* parser_;
        Span<const TokenType> follow_;

        bool first(TokenType type) { return parser_->can_begin_expression(type); }
        bool follow(TokenType type) { return contains(follow_, type); }
        std::unique_ptr<ast::Expr> parse() { return parser_->parse_expr_precedence(0, follow_); }

        std::string_view expected() { return "the start of an expression"; }
    };

    return check_error(Operation{this, follow});
}

/*
 * Recursive function that implements a pratt parser.
 * 
 * See also:
 *      http://crockford.com/javascript/tdop/tdop.html
 *      https://www.oilshell.org/blog/2016/11/01.html
 *      https://groups.google.com/forum/#!topic/comp.compilers/ruJLlQTVJ8o
 */
std::unique_ptr<ast::Expr> Parser::parse_expr_precedence(int min_precedence,
                                                                    Span<const TokenType> follow) {
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

        const int next_precedence = ast::operator_is_right_associative(*op) ? op_precedence
                                                                            : op_precedence + 1;
        binary_expr->right_child(parse_expr_precedence(next_precedence, follow));

        left = std::move(binary_expr);
    }

    return left;
}

/*
 * Parses a unary expressions. Unary expressions are either plain primary expressions
 * or a unary operator followed by another unary expression.
 */
std::unique_ptr<ast::Expr> Parser::parse_prefix_expr(Span<const TokenType> follow) {
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

std::unique_ptr<ast::Expr> Parser::parse_suffix_expr(Span<const TokenType> follow) {
    auto expr = parse_primary_expr(follow);
    if (!expr) {
        return nullptr;
    }

    return parse_suffix_expr_inner(std::move(expr));
}

std::unique_ptr<ast::Expr>
Parser::parse_suffix_expr_inner(std::unique_ptr<ast::Expr> current) {
    HAMMER_ASSERT_NOT_NULL(current);

    // Dot expr
    if (accept(TokenType::dot)) {
        auto dot = std::make_unique<ast::DotExpr>();
        dot->inner(std::move(current));

        if (auto ident_tok = expect(TokenType::identifier)) {
            dot->name(ident_tok->string_value());
            if (ident_tok->has_error()) {
                dot->has_error(true);
            }
        }

        return parse_suffix_expr_inner(std::move(dot));
    }

    // Call expr
    if (accept(TokenType::lparen)) {
        auto call = std::make_unique<ast::CallExpr>();
        call->func(std::move(current));

        if (!accept(TokenType::rparen)) {
            while (1) {
                if (auto arg = parse_expr({TokenType::rparen, TokenType::comma})) {
                    call->add_arg(std::move(arg));
                }

                if (accept(TokenType::rparen))
                    break;

                if (accept(TokenType::comma))
                    continue;

                diag_.reportf(
                    Diagnostics::error, current_.source(),
                    "Expected {} or {} in function argument list but encountered a {} instead.",
                    to_helpful_string(TokenType::comma), to_helpful_string(TokenType::rparen),
                    to_helpful_string(current_.type()));
                call->has_error(true);
                return call;
            }
        }

        return parse_suffix_expr_inner(std::move(call));
    }

    // Index expr
    if (accept(TokenType::lbracket)) {
        auto expr = std::make_unique<ast::IndexExpr>();
        expr->inner(std::move(current));

        if (auto arg = parse_expr({TokenType::rbracket})) {
            expr->index(std::move(arg));
        }

        if (!expect(TokenType::rbracket)) {
            expr->has_error(true);
            return expr;
        }

        return parse_suffix_expr_inner(std::move(expr));
    }

    return current;
}

std::unique_ptr<ast::Expr> Parser::parse_primary_expr(Span<const TokenType> follow) {
    switch (current_.type()) {

    // Block expr
    case TokenType::lbrace: {
        return parse_block_expr();
    }

    // Braced subexpression
    case TokenType::lparen: {
        advance();

        auto ex = parse_expr({TokenType::rparen});
        if (!expect(TokenType::rparen) && ex)
            ex->has_error(true);
        return ex;
    }

    // If expression
    case TokenType::kw_if: {
        return parse_if_expr();
    }

    // Return expression
    case TokenType::kw_return: {
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
    case TokenType::kw_continue:
        advance();
        return std::make_unique<ast::ContinueExpr>();

    // Break expression
    case TokenType::kw_break:
        advance();
        return std::make_unique<ast::BreakExpr>();

    // Variable reference
    case TokenType::identifier: {
        auto id = std::make_unique<ast::VarExpr>(current_.string_value());
        id->has_error(current_.has_error());
        advance();
        return id;
    }

    // Function Literal
    case TokenType::kw_func: {
        auto ret = std::make_unique<ast::FuncLiteral>();
        advance();

        auto func = parse_func_decl(false);
        ret->func(std::move(func));
        return ret;
    }

    // Null Literal
    case TokenType::kw_null: {
        auto lit = std::make_unique<ast::NullLiteral>();
        lit->has_error(current_.has_error());
        advance();
        return lit;
    }

    // Boolean literals
    case TokenType::kw_true:
    case TokenType::kw_false: {
        auto lit = std::make_unique<ast::BooleanLiteral>(current_.type() == TokenType::kw_true);
        advance();
        lit->has_error(current_.has_error());
        return lit;
    }

    // String literal
    case TokenType::string_literal: {
        auto str = std::make_unique<ast::StringLiteral>(current_.string_value());
        str->has_error(current_.has_error());
        advance();
        return str;
    }

    // Integer literal
    case TokenType::integer_literal: {
        auto lit = std::make_unique<ast::IntegerLiteral>(current_.int_value());
        lit->has_error(current_.has_error());
        advance();
        return lit;
    }

    // Float literal
    case TokenType::float_literal: {
        auto lit = std::make_unique<ast::FloatLiteral>(current_.float_value());
        lit->has_error(current_.has_error());
        advance();
        return lit;
    }

    default:
        break;
    }

    diag_.reportf(Diagnostics::error, current_.source(),
                  "Unexpected {}, expected a valid expression.",
                  to_helpful_string(current_.type()));
    return nullptr;
}

std::unique_ptr<ast::BlockExpr> Parser::parse_block_expr() {
    struct Operation {
        Parser* parser_;

        bool first(TokenType type) { return type == TokenType::lbrace; }

        bool follow(TokenType type) {
            unused(type);
            return false; /* scan until rbrace */
        }

        std::unique_ptr<ast::BlockExpr> parse() {
            if (!parser_->expect(TokenType::lbrace))
                return nullptr;

            std::unique_ptr<ast::BlockExpr> block = std::make_unique<ast::BlockExpr>();
            while (!parser_->accept(TokenType::rbrace)) {
                if (auto stmt = parser_->parse_stmt())
                    block->add_stmt(std::move(stmt));
            }

            return block;
        }

        std::string_view expected() { return to_helpful_string(TokenType::lbrace); }
    };

    return check_error(Operation{this});
}

std::unique_ptr<ast::IfExpr> Parser::parse_if_expr() {
    if (!expect(TokenType::kw_if))
        return nullptr;

    auto expr = std::make_unique<ast::IfExpr>();

    if (auto cond_expr = parse_expr({TokenType::lbrace})) {
        expr->condition(std::move(cond_expr));
    } else {
        expr->has_error(true);
    }

    if (auto then_expr = parse_block_expr()) {
        expr->then_branch(std::move(then_expr));
    } else {
        expr->has_error(true);
    }

    if (auto else_tok = accept(TokenType::kw_else)) {
        if (current_.type() == TokenType::kw_if) {
            expr->else_branch(parse_if_expr());
        } else {
            if (auto else_expr = parse_block_expr()) {
                expr->else_branch(std::move(else_expr));
            } else {
                expr->has_error(true);
            }
        }
    }
    return expr;
}

bool Parser::can_begin_var_decl(TokenType type) {
    return one_of(type, TokenType::kw_var, TokenType::kw_const);
}

bool Parser::can_begin_expression(TokenType type) {
    switch (type) {

    // Keywords
    case TokenType::kw_func:
    case TokenType::kw_continue:
    case TokenType::kw_break:
    case TokenType::kw_return:
    case TokenType::kw_if:
        return true;

    // Literal constants
    case TokenType::kw_true:
    case TokenType::kw_false:
    case TokenType::kw_null:
        return true;

    // Literal values
    case TokenType::identifier:
    case TokenType::string_literal:
    case TokenType::float_literal:
    case TokenType::integer_literal:
        return true;

    // ( expr )
    case TokenType::lparen:
        return true;

    // { statements ... }
    case TokenType::lbrace:
        return true;

    // Unary operators
    case TokenType::plus:
    case TokenType::minus:
    case TokenType::bnot:
    case TokenType::lnot:
        return true;

    default:
        return false;
    }
}

SourceReference Parser::ref(size_t begin, size_t end) const {
    return SourceReference::from_std_offsets(file_name_, begin, end);
}

std::optional<Token> Parser::accept(TokenType tok) {
    if (current_.type() == tok) {
        Token result = std::move(current_);
        advance();
        return {std::move(result)};
    }
    return {};
}

std::optional<Token> Parser::expect(TokenType tok) {
    auto res = accept(tok);
    if (!res) {
        diag_.reportf(Diagnostics::error, current_.source(), "Expected a {} but saw a {} instead.",
                      to_helpful_string(tok), to_helpful_string(current_.type()));
    }
    return res;
}

void Parser::advance() {
    current_ = lexer_.next();
}

} // namespace hammer
