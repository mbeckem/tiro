#include "hammer/compiler/syntax/parser.hpp"

#include "hammer/compiler/syntax/operators.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/math.hpp"

#include <fmt/format.h>

namespace hammer::compiler {

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
NodePtr<Node> Parser::make_node(const Token& start, Args&&... args) {
    static_assert(
        std::is_base_of_v<Node, Node>, "Node must be a derived class of Node.");

    auto node = std::make_shared<Node>(std::forward<Args>(args)...);
    node->start(start.source());
    if (start.has_error())
        node->has_error(true);
    return node;
}

template<typename Node>
Parser::Result<Node> Parser::result(NodePtr<Node>&& node, bool parse_ok) {
    if (!node)
        return error(std::move(node));
    if (!parse_ok)
        return error(std::move(node));
    return Result<Node>(std::move(node));
}

template<typename Node>
Parser::Result<Node> Parser::error(NodePtr<Node>&& node) {
    static_assert(std::is_base_of_v<Node, Node>);

    if (node)
        node->has_error(true);
    return Result<Node>(std::move(node), false);
}

template<typename Node, typename OtherNode>
Parser::Result<Node>
Parser::forward(NodePtr<Node>&& node, const Result<OtherNode>& other) {
    static_assert(std::is_base_of_v<Node, Node>);

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

template<typename Parse, typename Recover>
auto Parser::invoke(Parse&& parse, Recover&& recover) {
    auto r = parse();
    if (!r && recover()) {
        return result(r.take_node(), true);
    }
    return r;
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

Parser::Result<File> Parser::parse_file() {
    const Token start = current_;

    auto file = make_node<File>(start);
    file->file_name(file_name_);
    file->items(make_node<NodeList>(start));

    while (!accept(TokenType::Eof)) {
        if (auto brace = accept({TokenType::RightBrace, TokenType::RightBracket,
                TokenType::RightParen})) {
            diag_.reportf(Diagnostics::Error, brace->source(), "Unbalanced {}.",
                to_description(brace->type()));
            continue;
        }

        auto item = parse_toplevel_item({});
        if (item.has_node())
            file->items()->append(item.take_node());
        if (!item) {
            if (!recover_seek(TOPLEVEL_ITEM_FIRST, {})) {
                return error(std::move(file));
            }
        }
    }

    return file;
}

Parser::Result<Node> Parser::parse_toplevel_item(TokenTypes sync) {
    switch (current_.type()) {
    case TokenType::KwImport:
        return parse_import_decl(sync);
    case TokenType::KwFunc:
        return parse_func_decl(true, sync);
    case TokenType::Semicolon: {
        auto node = make_node<EmptyStmt>(current_);
        advance();
        return node;
    }
    default:
        diag_.reportf(Diagnostics::Error, current_.source(), "Unexpected {}.",
            to_description(current_.type()));
        return parse_failure;
    }
}

Parser::Result<ImportDecl> Parser::parse_import_decl(TokenTypes sync) {
    const auto start_tok = expect(TokenType::KwImport);
    if (!start_tok)
        return parse_failure;

    const auto parse = [&]() -> Result<ImportDecl> {
        auto decl = make_node<ImportDecl>(*start_tok);

        auto path_ok = [&]() {
            while (1) {
                auto ident = expect(TokenType::Identifier);
                if (!ident)
                    return false;

                decl->path_elements().push_back(ident->string_value());
                if (ident->has_error())
                    return false;

                if (!accept(TokenType::Dot))
                    return true;

                // Else: continue with identifier after dot.
            };
        }();

        if (decl->path_elements().size() > 0) {
            decl->name(decl->path_elements().back());
        }

        if (!path_ok)
            return error(std::move(decl));

        if (!expect(TokenType::Semicolon))
            return error(std::move(decl));

        return decl;
    };

    const auto recover = [&] {
        return recover_consume(TokenType::Semicolon, sync);
    };

    return invoke(parse, recover);
}

Parser::Result<FuncDecl>
Parser::parse_func_decl(bool requires_name, TokenTypes sync) {
    const auto start_tok = expect(TokenType::KwFunc);
    if (!start_tok)
        return parse_failure;

    auto func = make_node<FuncDecl>(*start_tok);

    if (auto ident = accept(TokenType::Identifier)) {
        func->name(ident->string_value());
        if (ident->has_error())
            func->has_error(true);
    } else if (requires_name) {
        diag_.reportf(Diagnostics::Error, current_.source(),
            "Expected a valid identifier for the new function's name but "
            "saw a {} instead.",
            to_description(current_.type()));
        func->has_error(true);
    }

    const auto params_start = expect(TokenType::LeftParen);
    if (!params_start)
        return error(std::move(func));

    func->params(make_node<ParamList>(*params_start));

    static constexpr ListOptions options{
        "parameter list", TokenType::RightParen};

    const bool list_ok = parse_braced_list(
        options, sync, [&]([[maybe_unused]] TokenTypes inner_sync) {
            auto param_ident = expect(TokenType::Identifier);
            if (param_ident) {
                auto param = make_node<ParamDecl>(*param_ident);
                param->name(param_ident->string_value());
                if (param_ident->has_error())
                    param->has_error(true);

                func->params()->append(std::move(param));
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

Parser::Result<Stmt> Parser::parse_stmt(TokenTypes sync) {
    if (auto empty_tok = accept(TokenType::Semicolon))
        return make_node<EmptyStmt>(*empty_tok);

    const TokenType type = current_.type();

    if (type == TokenType::KwAssert) {
        return parse_assert(sync);
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
        return parse_decl_stmt(sync);
    }

    if (can_begin_expression(type)) {
        return parse_expr_stmt(sync);
    }

    // Hint: can_begin_expression could be out of sync with
    // the expression parser.
    diag_.reportf(Diagnostics::Error, current_.source(),
        "Unexpected {} in statement context.", to_description(type));
    return parse_failure;
}

Parser::Result<AssertStmt> Parser::parse_assert(TokenTypes sync) {
    auto start_tok = expect(TokenType::KwAssert);
    if (!start_tok)
        return parse_failure;

    const auto parse = [&]() -> Result<AssertStmt> {
        auto stmt = make_node<AssertStmt>(*start_tok);

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
                        if (auto message = try_cast<StringLiteral>(node)) {
                            stmt->message(std::move(message));
                        } else {
                            diag_.reportf(Diagnostics::Error, node->start(),
                                "Expected a string literal.",
                                to_string(node->type()));
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

        if (!args_ok)
            return error(std::move(stmt));

        if (!expect(TokenType::Semicolon))
            return error(std::move(stmt));

        return stmt;
    };

    const auto recover = [&]() {
        return recover_consume(TokenType::Semicolon, sync);
    };

    return invoke(parse, recover);
}

Parser::Result<DeclStmt> Parser::parse_decl_stmt(TokenTypes sync) {
    const auto parse = [&]() -> Result<DeclStmt> {
        auto stmt = parse_var_decl(sync.union_with(TokenType::Semicolon));
        if (!stmt)
            return stmt;

        if (!expect(TokenType::Semicolon))
            return error(stmt.take_node());

        return stmt.take_node();
    };

    const auto recover = [&] {
        return recover_consume(TokenType::Semicolon, sync);
    };

    return invoke(parse, recover);
}

Parser::Result<DeclStmt> Parser::parse_var_decl(TokenTypes sync) {
    const auto decl_tok = expect(VAR_DECL_FIRST);
    if (!decl_tok)
        return parse_failure;

    auto stmt = make_node<DeclStmt>(*decl_tok);

    auto ident = accept(TokenType::Identifier);
    if (!ident) {
        diag_.reportf(Diagnostics::Error, current_.source(),
            "Unexpected {}, expected a valid identifier.",
            to_description(current_.type()));
        return error(std::move(stmt));
    }

    auto decl = make_node<VarDecl>(*ident);
    stmt->decl(decl);
    decl->is_const(decl_tok->type() == TokenType::KwConst);
    decl->name(ident->string_value());

    if (ident->has_error())
        return error(std::move(stmt));

    if (!accept(TokenType::Equals))
        return stmt;

    auto expr = parse_expr(sync);
    decl->initializer(expr.take_node());
    if (!expr)
        return error(std::move(stmt));

    return stmt;
}

Parser::Result<WhileStmt> Parser::parse_while_stmt(TokenTypes sync) {
    auto start_tok = expect(TokenType::KwWhile);
    if (!start_tok)
        return parse_failure;

    auto stmt = make_node<WhileStmt>(*start_tok);

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

Parser::Result<ForStmt> Parser::parse_for_stmt(TokenTypes sync) {
    auto start_tok = expect(TokenType::KwFor);
    if (!start_tok)
        return parse_failure;

    auto stmt = make_node<ForStmt>(*start_tok);

    if (!parse_for_stmt_header(stmt.get(), sync))
        return error(std::move(stmt));

    auto body = parse_block_expr(sync);
    stmt->body(body.take_node());
    return forward(std::move(stmt), body);
}

bool Parser::parse_for_stmt_header(ForStmt* stmt, TokenTypes sync) {
    const bool has_parens = static_cast<bool>(accept(TokenType::LeftParen));

    const auto parse_init = [&] {
        const auto parse = [&]() -> Result<DeclStmt> {
            if (!can_begin_var_decl(current_.type())) {
                diag_.reportf(Diagnostics::Error, current_.source(),
                    "Expected a variable declaration or a {}.",
                    to_description(TokenType::Semicolon));
                return parse_failure;
            }

            auto decl = parse_var_decl(sync.union_with(TokenType::Semicolon));
            if (!decl)
                return decl;

            if (!expect(TokenType::Semicolon))
                return error(decl.take_node());

            return decl;
        };

        const auto recover = [&] {
            return recover_consume(TokenType::Semicolon, sync).has_value();
        };

        return invoke(parse, recover);
    };

    const auto parse_condition = [&] {
        const auto parse = [&]() -> Result<Expr> {
            auto expr = parse_expr(sync.union_with(TokenType::Semicolon));
            if (!expr)
                return expr;

            if (!expect(TokenType::Semicolon))
                return error(expr.take_node());

            return expr;
        };

        const auto recover = [&] {
            return recover_consume(TokenType::Semicolon, sync).has_value();
        };

        return invoke(parse, recover);
    };

    const auto parse_step = [&](TokenType next) {
        const auto parse = [&]() -> Result<Expr> {
            return parse_expr(sync.union_with(next));
        };

        const auto recover = [&]() { return recover_seek(next, sync); };

        return invoke(parse, recover);
    };

    const auto parse = [&] {
        // Optional init statement
        if (!accept(TokenType::Semicolon)) {
            auto init = parse_init();
            stmt->decl(init.take_node());
            if (!init)
                return false;
        }

        // Optional condition expression
        if (!accept(TokenType::Semicolon)) {
            auto cond = parse_condition();
            stmt->condition(cond.take_node());
            if (!cond)
                return false;
        }

        // Optional step expression
        const TokenType next = has_parens ? TokenType::RightParen
                                          : TokenType::LeftBrace;
        if (current_.type() != next) {
            auto step = parse_step(next);
            stmt->step(step.take_node());
            if (!step)
                return false;
        }

        if (has_parens) {
            if (!expect(TokenType::RightParen))
                return false;
        }

        return true;
    };

    const auto recover = [&] {
        return has_parens
                   ? recover_consume(TokenType::RightParen, sync).has_value()
                   : recover_seek(TokenType::LeftBrace, sync);
    };

    if (!parse()) {
        stmt->has_error(true);
        return recover() ? true : false;
    }
    return true;
}

Parser::Result<ExprStmt> Parser::parse_expr_stmt(TokenTypes sync) {
    const bool need_semicolon = !EXPR_STMT_OPTIONAL_SEMICOLON.contains(
        current_.type());

    const auto parse = [&]() -> Result<ExprStmt> {
        auto stmt = make_node<ExprStmt>(current_);

        auto expr = parse_expr(sync.union_with(TokenType::Semicolon));
        stmt->expr(expr.take_node());
        if (!expr)
            return error(std::move(stmt));

        if (need_semicolon) {
            if (!expect(TokenType::Semicolon))
                return error(std::move(stmt));
        } else {
            accept(TokenType::Semicolon);
        }
        return stmt;
    };

    const auto recover = [&] {
        return recover_consume(TokenType::Semicolon, sync);
    };

    return invoke(parse, recover);
}

Parser::Result<Expr> Parser::parse_expr(TokenTypes sync) {
    return parse_expr(0, sync);
}

/// Recursive function that implements a pratt parser.
///
/// See also:
///      http://crockford.com/javascript/tdop/tdop.html
///      https://www.oilshell.org/blog/2016/11/01.html
///      https://groups.google.com/forum/#!topic/comp.compilers/ruJLlQTVJ8o
Parser::Result<Expr> Parser::parse_expr(int min_precedence, TokenTypes sync) {
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

Parser::Result<Expr> Parser::parse_infix_expr(
    NodePtr<Expr> left, int current_precedence, TokenTypes sync) {

    if (auto op = to_binary_operator(current_.type())) {
        auto binary_expr = make_node<BinaryExpr>(current_, *op);
        advance();
        binary_expr->left(std::move(left));

        int next_precedence = current_precedence;
        if (!operator_is_right_associative(*op))
            ++next_precedence;

        auto right = parse_expr(next_precedence, sync);
        binary_expr->right(right.take_node());
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

/// Parses a unary expressions. Unary expressions are either plain primary
/// expressions or a unary operator followed by another unary expression.
Parser::Result<Expr> Parser::parse_prefix_expr(TokenTypes sync) {
    auto op = to_unary_operator(current_.type());
    if (!op)
        return parse_primary_expr(sync);

    // It's a unary operator
    auto unary = make_node<UnaryExpr>(current_, *op);
    advance();

    auto inner = parse_expr(unary_precedence, sync);
    unary->inner(inner.take_node());
    return forward(std::move(unary), inner);
}

Parser::Result<DotExpr> Parser::parse_member_expr(
    NodePtr<Expr> current, [[maybe_unused]] TokenTypes sync) {
    auto start_tok = expect(TokenType::Dot);
    if (!start_tok)
        return parse_failure;

    auto dot = make_node<DotExpr>(*start_tok);
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

Parser::Result<CallExpr>
Parser::parse_call_expr(NodePtr<Expr> current, TokenTypes sync) {
    auto start_tok = expect(TokenType::LeftParen);
    if (!start_tok)
        return parse_failure;

    auto call = make_node<CallExpr>(*start_tok);
    call->func(std::move(current));
    call->args(make_node<ExprList>(*start_tok));

    static constexpr ListOptions options{
        "argument list", TokenType::RightParen};
    const bool list_ok = parse_braced_list(
        options, sync, [&](TokenTypes inner_sync) {
            auto arg = parse_expr(inner_sync);
            if (arg.has_node())
                call->args()->append(arg.take_node());

            return arg.parse_ok();
        });

    return result(std::move(call), list_ok);
}

Parser::Result<IndexExpr>
Parser::parse_index_expr(NodePtr<Expr> current, TokenTypes sync) {
    auto start_tok = expect(TokenType::LeftBracket);
    if (!start_tok)
        return parse_failure;

    const auto parse = [&]() -> Result<IndexExpr> {
        auto expr = make_node<IndexExpr>(*start_tok);
        expr->inner(std::move(current));

        auto index = parse_expr(TokenType::RightBracket);
        expr->index(index.take_node());
        if (!index)
            return error(std::move(expr));

        if (!expect(TokenType::RightBracket))
            return error(std::move(expr));

        return expr;
    };

    const auto recover = [&] {
        return recover_consume(TokenType::RightBracket, sync);
    };

    return invoke(parse, recover);
}

Parser::Result<Expr> Parser::parse_primary_expr(TokenTypes sync) {
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
        auto ret = make_node<ReturnExpr>(current_);
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
        auto cont = make_node<ContinueExpr>(current_);
        advance();
        return cont;
    }

    // Break expression
    case TokenType::KwBreak: {
        auto brk = make_node<BreakExpr>(current_);
        advance();
        return brk;
    }

    // Variable reference
    case TokenType::Identifier: {
        const bool has_error = current_.has_error();
        auto id = make_node<VarExpr>(current_, current_.string_value());
        advance();
        return result(std::move(id), !has_error);
    }

    // Function Literal
    case TokenType::KwFunc: {
        auto ret = make_node<FuncLiteral>(current_);

        auto func = parse_func_decl(false, sync);
        ret->func(func.take_node());
        if (!func)
            return error(std::move(ret));

        return ret;
    }

    // Array literal.
    case TokenType::LeftBracket: {
        const auto start = current_;

        auto lit = make_node<ArrayLiteral>(start);
        advance();

        static constexpr auto options = ListOptions(
            "array literal", TokenType::RightBracket)
                                            .set_allow_trailing_comma(true);

        lit->entries(make_node<ExprList>(start));
        const bool list_ok = parse_braced_list(
            options, sync, [&](TokenTypes inner_sync) {
                auto value = parse_expr(inner_sync);
                if (value.has_node())
                    lit->entries()->append(value.take_node());

                return value.parse_ok();
            });

        return result(std::move(lit), list_ok);
    }

    // Map literal
    case TokenType::KwMap: {
        const auto start = current_;

        auto lit = make_node<MapLiteral>(start);
        advance();

        const auto entries_start = expect(TokenType::LeftBrace);
        if (!entries_start)
            return error(std::move(lit));

        lit->entries(make_node<MapEntryList>(*entries_start));

        static constexpr auto options = ListOptions(
            "map literal", TokenType::RightBrace)
                                            .set_allow_trailing_comma(true);

        auto parse_entry = [&](TokenTypes entry_sync) -> Result<MapEntry> {
            auto entry = make_node<MapEntry>(current_);

            {
                auto key = parse_expr(entry_sync.union_with(TokenType::Colon));
                if (key.has_node())
                    entry->key(key.take_node());
                if (!key)
                    return error(std::move(entry));
            }

            if (!expect(TokenType::Colon))
                return error(std::move(entry));

            {
                auto value = parse_expr(entry_sync);
                if (value.has_node())
                    entry->value(value.take_node());
                if (!value)
                    return error(std::move(entry));
            }

            return entry;
        };

        const bool list_ok = parse_braced_list(
            options, sync, [&](TokenTypes inner_sync) {
                auto entry = parse_entry(inner_sync);
                if (entry.has_node()) {
                    lit->entries()->append(entry.take_node());
                }

                return entry.parse_ok();
            });

        return result(std::move(lit), list_ok);
    }

    // Set literal
    case TokenType::KwSet: {
        const auto start = current_;

        auto lit = make_node<SetLiteral>(start);
        advance();

        const auto entries_start = expect(TokenType::LeftBrace);
        if (!entries_start)
            return error(std::move(lit));

        lit->entries(make_node<ExprList>(*entries_start));

        static constexpr auto options = ListOptions(
            "set literal", TokenType::RightBrace)
                                            .set_allow_trailing_comma(true);

        const bool list_ok = parse_braced_list(
            options, sync, [&](TokenTypes inner_sync) {
                auto value = parse_expr(inner_sync);
                if (value.has_node())
                    lit->entries()->append(value.take_node());

                return value.parse_ok();
            });

        return result(std::move(lit), list_ok);
    }

    // Null Literal
    case TokenType::KwNull: {
        auto lit = make_node<NullLiteral>(current_);
        lit->has_error(current_.has_error());
        advance();
        return lit;
    }

    // Boolean literals
    case TokenType::KwTrue:
    case TokenType::KwFalse: {
        auto lit = make_node<BooleanLiteral>(
            current_, current_.type() == TokenType::KwTrue);
        advance();
        lit->has_error(current_.has_error());
        return lit;
    }

    // String literal(s)
    case TokenType::StringLiteral: {
        const auto first_string = current_;

        auto str = make_node<StringLiteral>(current_, current_.string_value());
        advance();

        if (str->has_error())
            return str;

        // Adjacent string literals are grouped together in a sequence.
        if (current_.type() == TokenType::StringLiteral) {
            auto seq = make_node<StringSequenceExpr>(first_string);
            auto strings = make_node<ExprList>(first_string);
            seq->strings(strings);
            strings->append(std::move(str));

            do {
                str = make_node<StringLiteral>(
                    current_, current_.string_value());
                advance();

                if (str->has_error())
                    seq->has_error(true);

                strings->append(std::move(str));
            } while (!seq->has_error()
                     && current_.type() == TokenType::StringLiteral);
            return seq;
        }
        return str;
    }

    // Symbol literal
    case TokenType::SymbolLiteral: {
        auto sym = make_node<SymbolLiteral>(current_, current_.string_value());
        sym->has_error(current_.has_error());
        advance();
        return sym;
    }

    // Integer literal
    case TokenType::IntegerLiteral: {
        auto lit = make_node<IntegerLiteral>(current_, current_.int_value());
        lit->has_error(current_.has_error());
        advance();
        return lit;
    }

    // Float literal
    case TokenType::FloatLiteral: {
        auto lit = make_node<FloatLiteral>(current_, current_.float_value());
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
    return parse_failure;
}

Parser::Result<BlockExpr> Parser::parse_block_expr(TokenTypes sync) {
    auto start_tok = expect(TokenType::LeftBrace);
    if (!start_tok)
        return parse_failure;

    const auto parse = [&]() -> Result<BlockExpr> {
        auto block = make_node<BlockExpr>(*start_tok);
        auto stmts = make_node<StmtList>(*start_tok);
        block->stmts(stmts);

        while (!accept(TokenType::RightBrace)) {
            if (current_.type() == TokenType::Eof) {
                diag_.reportf(Diagnostics::Error, current_.source(),
                    "Unterminated block expression, expected {}.",
                    to_description(TokenType::RightBrace));
                return error(std::move(block));
            }

            auto stmt = parse_stmt(sync.union_with(TokenType::RightBrace));
            if (stmt.has_node())
                stmts->append(stmt.take_node());

            if (!stmt)
                return error(std::move(block));
        }

        return block;
    };

    const auto recover = [&] {
        return recover_consume(TokenType::RightBrace, sync);
    };

    return invoke(parse, recover);
}

Parser::Result<IfExpr> Parser::parse_if_expr(TokenTypes sync) {
    auto start_tok = expect(TokenType::KwIf);
    if (!start_tok)
        return parse_failure;

    auto expr = make_node<IfExpr>(*start_tok);

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

Parser::Result<Expr> Parser::parse_paren_expr(TokenTypes sync) {
    auto start_tok = expect(TokenType::LeftParen);
    if (!start_tok)
        return parse_failure;

    const auto parse = [&]() -> Result<Expr> {
        // "()" is the empty tuple.
        if (accept(TokenType::RightParen)) {
            auto tuple = make_node<TupleLiteral>(*start_tok);
            auto entries = make_node<ExprList>(*start_tok);
            tuple->entries(entries);
            return tuple;
        }

        // Parse the initial expression - don't know whether this is a tuple yet.
        auto expr = parse_expr(
            sync.union_with({TokenType::Comma, TokenType::RightParen}));
        if (!expr)
            return expr;

        auto initial = expr.take_node();

        auto next = expect({TokenType::Comma, TokenType::RightParen});
        if (!next)
            return error(std::move(initial));

        // "(expr)" is not a tuple.
        if (next->type() == TokenType::RightParen)
            return initial;

        // "(expr, ..." is guaranteed to be a tuple.
        if (next->type() == TokenType::Comma)
            return parse_tuple(*start_tok, std::move(initial), sync);

        HAMMER_UNREACHABLE("Invalid token type.");
    };

    const auto recover = [&] {
        return recover_consume(TokenType::RightParen, sync);
    };

    return invoke(parse, recover);
}

Parser::Result<TupleLiteral> Parser::parse_tuple(
    const Token& start_tok, NodePtr<Expr> first_item, TokenTypes sync) {
    auto tuple = make_node<TupleLiteral>(start_tok);
    tuple->entries(make_node<ExprList>(start_tok));

    if (first_item)
        tuple->entries()->append(std::move(first_item));

    static constexpr auto options = ListOptions(
        "tuple literal", TokenType::RightParen)
                                        .set_allow_trailing_comma(true);

    const bool list_ok = parse_braced_list(
        options, sync, [&](TokenTypes inner_sync) {
            auto expr = parse_expr(inner_sync);
            if (expr.has_node())
                tuple->entries()->append(expr.take_node());
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

} // namespace hammer::compiler
