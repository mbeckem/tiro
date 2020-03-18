#include "tiro/syntax/parser.hpp"

#include "tiro/core/defs.hpp"
#include "tiro/core/math.hpp"
#include "tiro/syntax/operators.hpp"

#include <fmt/format.h>

namespace tiro {

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

static const TokenTypes STRING_FIRST = {
    TokenType::SingleQuote, TokenType::DoubleQuote};

// Important: all token types that can be a legal beginning of an expression
// MUST be listed here. Otherwise, the expression parser will bail out immediately,
// even if the token would be handled somewhere down in the implementation!
static const TokenTypes EXPR_FIRST =
    TokenTypes{
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
    }
        .union_with(STRING_FIRST);

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

static bool can_begin_var_decl(TokenType type) {
    return VAR_DECL_FIRST.contains(type);
}

static bool can_begin_expression(TokenType type) {
    return EXPR_FIRST.contains(type);
}

static bool can_begin_string(TokenType type) {
    return STRING_FIRST.contains(type);
}

template<typename Node, typename... Args>
NodePtr<Node> Parser::make_node(const Token& start, Args&&... args) {
    static_assert(
        std::is_base_of_v<Node, Node>, "Node must be a derived class of Node.");

    auto node = make_ref<Node>(std::forward<Args>(args)...);
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

bool Parser::parse_braced_list(const ListOptions& options, TokenTypes sync,
    FunctionRef<bool(TokenTypes inner_sync)> parser) {
    TIRO_ASSERT(!options.name.empty(), "Must not have an empty name.");
    TIRO_ASSERT(options.right_brace != TokenType::InvalidToken,
        "Must set the right brace token type.");
    TIRO_ASSERT(options.max_count == -1 || options.max_count >= 0,
        "Invalid max count.");

    int current_count = 0;

    if (accept(options.right_brace))
        return true;

    const auto inner_sync = sync.union_with(
        {TokenType::Comma, options.right_brace});

    while (1) {
        {
            const Token& current = head();
            if (current.type() == TokenType::Eof) {
                diag_.reportf(Diagnostics::Error, current.source(),
                    "Unterminated {}, expected {}.", options.name,
                    to_description(options.right_brace));
                return false;
            }

            if (options.max_count != -1 && current_count >= options.max_count) {
                // TODO: Proper recovery until "," or brace?
                diag_.reportf(Diagnostics::Error, current.source(),
                    "Unexpected {} in {}, expected {}.",
                    to_description(current.type()), options.name,
                    to_description(options.right_brace));
                return false;
            }
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

        TIRO_UNREACHABLE("Invalid token type.");
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
    const Token start = head();

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
    const Token start = head();
    switch (start.type()) {
    case TokenType::KwImport:
        return parse_import_decl(sync);
    case TokenType::KwFunc:
        return parse_func_decl(true, sync);
    case TokenType::Semicolon: {
        auto node = make_node<EmptyStmt>(start);
        advance();
        return node;
    }
    default:
        break;
    }

    if (can_begin_var_decl(start.type()))
        return parse_decl_stmt(sync);

    diag_.reportf(Diagnostics::Error, start.source(), "Unexpected {}.",
        to_description(start.type()));
    return parse_failure;
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

        if (!decl->path_elements().empty()) {
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
        const Token& tok = head();
        diag_.reportf(Diagnostics::Error, tok.source(),
            "Expected a valid identifier for the new function's name but "
            "saw a {} instead.",
            to_description(tok.type()));
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
            }
            return false;
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

    const TokenType type = head().type();

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
    diag_.reportf(Diagnostics::Error, head().source(),
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
                        if (auto message = try_cast<InterpolatedStringExpr>(
                                node)) {
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
                    TIRO_UNREACHABLE(
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

    const bool is_const = decl_tok->type() == TokenType::KwConst;

    auto stmt = make_node<DeclStmt>(*decl_tok);
    auto bindings = make_node<BindingList>(*decl_tok);
    stmt->bindings(bindings);

    while (1) {
        auto binding = parse_binding(is_const, sync);
        bindings->append(binding.take_node());
        if (!binding)
            return error(std::move(stmt));

        if (!accept(TokenType::Comma))
            break;
    }

    return stmt;
}

Parser::Result<Binding> Parser::parse_binding(bool is_const, TokenTypes sync) {
    auto lhs = parse_binding_lhs(is_const, sync);
    if (!lhs)
        return lhs;

    auto binding = lhs.take_node();
    if (!accept(TokenType::Equals))
        return binding;

    auto expr = parse_expr(sync);
    binding->init(expr.take_node());
    if (!expr)
        return error(std::move(binding));

    return binding;
}

Parser::Result<Binding>
Parser::parse_binding_lhs(bool is_const, TokenTypes sync) {
    auto next = accept({TokenType::Identifier, TokenType::LeftParen});
    if (!next) {
        const Token& tok = head();
        diag_.reportf(Diagnostics::Error, tok.source(),
            "Unexpected {}, expected a valid identifier or a '('.",
            to_description(tok.type()));
        return parse_failure;
    }

    if (next->type() == TokenType::LeftParen) {
        static constexpr auto options = ListOptions(
            "tuple declaration", TokenType::RightParen)
                                            .set_allow_trailing_comma(true);

        auto binding = make_node<TupleBinding>(*next);
        auto vars = make_node<VarList>(*next);
        binding->vars(vars);

        const bool list_ok = parse_braced_list(
            options, sync, [&]([[maybe_unused]] TokenTypes inner_sync) {
                auto ident = accept(TokenType::Identifier);
                if (!ident) {
                    const Token& tok = head();
                    diag_.reportf(Diagnostics::Error, tok.source(),
                        "Unexpected {}, expected a valid identifier.",
                        to_description(tok.type()));
                    return false;
                }

                auto decl = make_node<VarDecl>(*ident);
                decl->name(ident->string_value());
                decl->is_const(is_const);
                vars->append(decl);

                if (ident->has_error()) {
                    decl->has_error(true);
                    return false;
                }
                return true;
            });

        if (!list_ok)
            return error(std::move(binding));

        if (vars->size() == 0) {
            diag_.report(Diagnostics::Error, vars->start(),
                "Variable lists must not be empty in tuple unpacking "
                "declarations.");
            binding->has_error(true);
            // Parser is still ok - just report the grammar error
        }
        return binding;
    }

    if (next->type() == TokenType::Identifier) {
        auto binding = make_node<VarBinding>(*next);

        auto decl = make_node<VarDecl>(*next);
        decl->name(next->string_value());
        decl->is_const(is_const);
        binding->var(decl);

        if (next->has_error()) {
            decl->has_error(true);
            return error(std::move(binding));
        }

        return binding;
    }

    TIRO_UNREACHABLE("Invalid token type.");
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

    if (head().type() != TokenType::LeftBrace) {
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
            if (const Token& tok = head(); !can_begin_var_decl(tok.type())) {
                diag_.reportf(Diagnostics::Error, tok.source(),
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
        if (head().type() != next) {
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
        return recover();
    }
    return true;
}

Parser::Result<ExprStmt> Parser::parse_expr_stmt(TokenTypes sync) {
    Token start_tok = head();

    const bool need_semicolon = !EXPR_STMT_OPTIONAL_SEMICOLON.contains(
        start_tok.type());

    const auto parse = [&]() -> Result<ExprStmt> {
        auto stmt = make_node<ExprStmt>(start_tok);

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
        const int op_precedence = infix_operator_precedence(head().type());
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

Parser::Result<Expr>
Parser::parse_infix_expr(Expr* left, int current_precedence, TokenTypes sync) {

    Token current = head();

    if (auto op = to_binary_operator(current.type())) {
        auto binary_expr = make_node<BinaryExpr>(current, *op);
        advance();
        binary_expr->left(left);

        int next_precedence = current_precedence;
        if (!operator_is_right_associative(*op))
            ++next_precedence;

        auto right = parse_expr(next_precedence, sync);
        binary_expr->right(right.take_node());
        return forward(std::move(binary_expr), right);
    } else if (current.type() == TokenType::LeftParen) {
        return parse_call_expr(left, sync);
    } else if (current.type() == TokenType::LeftBracket) {
        return parse_index_expr(left, sync);
    } else if (current.type() == TokenType::Dot) {
        return parse_member_expr(left, sync);
    } else {
        TIRO_ERROR("Invalid operator in parse_infix_operator: {}",
            to_description(current.type()));
    }
}

/// Parses a unary expressions. Unary expressions are either plain primary
/// expressions or a unary operator followed by another unary expression.
Parser::Result<Expr> Parser::parse_prefix_expr(TokenTypes sync) {
    Token current = head();

    auto op = to_unary_operator(current.type());
    if (!op)
        return parse_primary_expr(sync);

    // It's a unary operator
    auto unary = make_node<UnaryExpr>(current, *op);
    advance();

    auto inner = parse_expr(unary_precedence, sync);
    unary->inner(inner.take_node());
    return forward(std::move(unary), inner);
}

Parser::Result<Expr>
Parser::parse_member_expr(Expr* current, [[maybe_unused]] TokenTypes sync) {
    auto start_tok = expect(TokenType::Dot);
    if (!start_tok)
        return parse_failure;

    auto reset_mode = enter_lexer_mode(LexerMode::Member);

    auto member_tok = expect({TokenType::Identifier, TokenType::NumericMember});
    if (!member_tok)
        return error(ref(current));

    if (member_tok->type() == TokenType::Identifier) {
        auto dot = make_node<DotExpr>(*start_tok);
        dot->inner(current);
        dot->name(member_tok->string_value());
        if (member_tok->has_error())
            return error(std::move(dot));

        return dot;
    }

    if (member_tok->type() == TokenType::NumericMember) {
        auto tup = make_node<TupleMemberExpr>(*start_tok);
        tup->inner(current);

        const i64 value = member_tok->int_value();
        if (value < 0 || value > std::numeric_limits<u32>::max()) {
            diag_.reportf(Diagnostics::Error, member_tok->source(),
                "Integer value {} cannot be used as a tuple member index.",
                value);
            return error(std::move(tup));
        }

        tup->index(static_cast<u32>(value));
        if (member_tok->has_error())
            return error(std::move(tup));

        return tup;
    }

    TIRO_UNREACHABLE("Invalid token type.");
}

Parser::Result<CallExpr>
Parser::parse_call_expr(Expr* current, TokenTypes sync) {
    auto start_tok = expect(TokenType::LeftParen);
    if (!start_tok)
        return parse_failure;

    auto call = make_node<CallExpr>(*start_tok);
    call->func(current);
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
Parser::parse_index_expr(Expr* current, TokenTypes sync) {
    auto start_tok = expect(TokenType::LeftBracket);
    if (!start_tok)
        return parse_failure;

    const auto parse = [&]() -> Result<IndexExpr> {
        auto expr = make_node<IndexExpr>(*start_tok);
        expr->inner(current);

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
    Token start = head();

    if (can_begin_string(start.type()))
        return parse_string_sequence(sync);

    switch (start.type()) {
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
        auto ret = make_node<ReturnExpr>(start);
        advance();

        if (can_begin_expression(head().type())) {
            auto inner = parse_expr(sync);
            ret->inner(inner.take_node());
            if (!inner)
                return error(std::move(ret));
        }
        return ret;
    }

    // Continue expression
    case TokenType::KwContinue: {
        auto cont = make_node<ContinueExpr>(start);
        advance();
        return cont;
    }

    // Break expression
    case TokenType::KwBreak: {
        auto brk = make_node<BreakExpr>(start);
        advance();
        return brk;
    }

    // Variable reference
    case TokenType::Identifier: {
        return parse_identifier(sync);
    }

    // Function Literal
    case TokenType::KwFunc: {
        auto ret = make_node<FuncLiteral>(start);

        auto func = parse_func_decl(false, sync);
        ret->func(func.take_node());
        if (!func)
            return error(std::move(ret));

        return ret;
    }

    // Array literal.
    case TokenType::LeftBracket: {
        auto lit = make_node<ArrayLiteral>(start);
        auto entries = make_node<ExprList>(start);
        lit->entries(entries);
        advance();

        static constexpr auto options = ListOptions(
            "array literal", TokenType::RightBracket)
                                            .set_allow_trailing_comma(true);

        const bool list_ok = parse_braced_list(
            options, sync, [&](TokenTypes inner_sync) {
                auto value = parse_expr(inner_sync);
                if (value.has_node())
                    entries->append(value.take_node());

                return value.parse_ok();
            });

        return result(std::move(lit), list_ok);
    }

    // Map literal
    case TokenType::KwMap: {
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
            auto entry = make_node<MapEntry>(head());

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
        auto lit = make_node<NullLiteral>(start);
        lit->has_error(start.has_error());
        advance();
        return lit;
    }

    // Boolean literals
    case TokenType::KwTrue:
    case TokenType::KwFalse: {
        auto lit = make_node<BooleanLiteral>(
            start, start.type() == TokenType::KwTrue);
        lit->has_error(start.has_error());
        advance();
        return lit;
    }

    // Symbol literal
    case TokenType::SymbolLiteral: {
        auto sym = make_node<SymbolLiteral>(start, start.string_value());
        sym->has_error(start.has_error());
        advance();
        return sym;
    }

    // Integer literal
    case TokenType::IntegerLiteral: {
        auto lit = make_node<IntegerLiteral>(start, start.int_value());
        lit->has_error(start.has_error());
        advance();
        return lit;
    }

    // Float literal
    case TokenType::FloatLiteral: {
        auto lit = make_node<FloatLiteral>(start, start.float_value());
        lit->has_error(start.has_error());
        advance();
        return lit;
    }

    default:
        break;
    }

    diag_.reportf(Diagnostics::Error, start.source(),
        "Unexpected {}, expected a valid expression.",
        to_description(start.type()));
    return parse_failure;
}

Parser::Result<VarExpr>
Parser::parse_identifier([[maybe_unused]] TokenTypes sync) {
    auto tok = expect(TokenType::Identifier);
    if (!tok)
        return parse_failure;

    const bool has_error = tok->has_error();
    auto id = make_node<VarExpr>(*tok, tok->string_value());
    return result(std::move(id), !has_error);
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
            if (const Token& tok = head(); tok.type() == TokenType::Eof) {
                diag_.reportf(Diagnostics::Error, tok.source(),
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
        if (head().type() == TokenType::KwIf) {
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

        TIRO_UNREACHABLE("Invalid token type.");
    };

    const auto recover = [&] {
        return recover_consume(TokenType::RightParen, sync);
    };

    return invoke(parse, recover);
}

Parser::Result<TupleLiteral>
Parser::parse_tuple(const Token& start_tok, Expr* first_item, TokenTypes sync) {
    auto tuple = make_node<TupleLiteral>(start_tok);
    tuple->entries(make_node<ExprList>(start_tok));

    if (first_item)
        tuple->entries()->append(first_item);

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

Parser::Result<Expr> Parser::parse_string_sequence(TokenTypes sync) {
    const auto start = head();

    auto str_result = parse_string_expr(sync);
    if (!str_result || !str_result.has_node())
        return str_result;

    auto str = str_result.take_node();

    // Adjacent string literals are grouped together in a sequence.
    if (can_begin_string(head().type())) {
        auto seq = make_node<StringSequenceExpr>(start);
        auto strings = make_node<ExprList>(start);
        seq->strings(strings);
        strings->append(std::move(str));

        while (1) {
            auto next_str_result = parse_string_expr(sync);
            if (next_str_result.has_node())
                strings->append(next_str_result.take_node());
            if (!next_str_result)
                return error(std::move(seq));

            if (!can_begin_string(head().type()))
                break;
        }

        return seq;
    }

    return str;
}

Parser::Result<Expr> Parser::parse_string_expr(TokenTypes sync) {
    auto start_tok = expect({TokenType::SingleQuote, TokenType::DoubleQuote});
    if (!start_tok)
        return parse_failure;

    const auto end_type = start_tok->type();
    const auto lexer_mode = start_tok->type() == TokenType::SingleQuote
                                ? LexerMode::StringSingleQuote
                                : LexerMode::StringDoubleQuote;

    const auto interpolated_mode = enter_lexer_mode(lexer_mode);

    const auto parse = [&]() -> Result<Expr> {
        auto expr = make_node<InterpolatedStringExpr>(*start_tok);
        auto items = make_node<ExprList>(*start_tok);
        expr->items(items);

        while (1) {
            auto item_tok = expect({TokenType::StringContent, TokenType::Dollar,
                TokenType::DollarLeftBrace, end_type});
            if (!item_tok)
                return error(std::move(expr));

            if (item_tok->type() == end_type)
                break;

            if (item_tok->type() == TokenType::StringContent) {
                auto str = make_node<StringLiteral>(
                    *item_tok, item_tok->string_value());
                items->append(str);
                if (str->has_error())
                    return error(std::move(expr));

                continue;
            }

            auto item_expr = parse_interpolated_expr(
                item_tok->type(), sync.union_with(end_type));
            if (item_expr.has_node())
                items->append(item_expr.take_node());
            if (!item_expr)
                return error(std::move(expr));

            // Else: continue with next iteration, lexer mode is restored
        }

        return expr;
    };

    const auto recover = [&] { return recover_consume(end_type, sync); };

    return invoke(parse, recover);
}

Parser::Result<Expr>
Parser::parse_interpolated_expr(TokenType starter, TokenTypes sync) {
    TIRO_ASSERT(
        starter == TokenType::Dollar || starter == TokenType::DollarLeftBrace,
        "Must start with $ or ${.");

    const auto normal_mode = enter_lexer_mode(LexerMode::Normal);
    const auto peek = head();

    if (starter == TokenType::Dollar) {
        if (peek.type() != TokenType::Identifier) {
            diag_.reportf(Diagnostics::Error, peek.source(),
                "Unexpected {}, expected an identifier. Use '${{' (no "
                "space) to include a complex expression or use '\\$' to escape "
                "the dollar sign.",
                to_description(peek.type()));
            return parse_failure;
        }

        return parse_identifier(sync);
    }

    if (starter == TokenType::DollarLeftBrace) {
        auto parse = [&]() -> Result<Expr> {
            auto expr = parse_expr(sync.union_with(TokenType::RightBrace));
            if (!expr)
                return expr;

            if (!expect(TokenType::RightBrace))
                return error(expr.take_node());

            return expr;
        };

        auto recover = [&]() {
            return recover_consume(TokenType::RightBrace, sync);
        };

        return invoke(parse, recover);
    }

    TIRO_UNREACHABLE("Invalid token type to start an interpolated expression.");
}

Token& Parser::head() {
    if (!head_) {
        head_ = lexer_.next();
    }
    return *head_;
}

void Parser::advance() {
    head_ = std::nullopt;
}

std::optional<Token> Parser::accept(TokenTypes tokens) {
    if (Token& peek = head(); tokens.contains(peek.type())) {
        Token tok = std::move(peek);
        advance();
        return {std::move(tok)};
    }
    return {};
}

std::optional<Token> Parser::expect(TokenTypes tokens) {
    TIRO_ASSERT(!tokens.empty(), "Token set must not be empty.");

    auto res = accept(tokens);
    if (!res) {
        const Token& tok = head();
        diag_.report(Diagnostics::Error, tok.source(),
            unexpected_message({}, tokens, tok.type()));
    }
    return res;
}

bool Parser::recover_seek(TokenTypes expected, TokenTypes sync) {
    // TODO: It might be useful to track opening / closing braces in here?
    // We might be skipping over them otherwise.
    while (1) {
        const Token& tok = head();

        if (tok.type() == TokenType::Eof
            || tok.type() == TokenType::InvalidToken)
            return false;

        if (expected.contains(tok.type()))
            return true;

        if (sync.contains(tok.type()))
            return false;

        advance();
    }
}

std::optional<Token>
Parser::recover_consume(TokenTypes expected, TokenTypes sync) {
    if (recover_seek(expected, sync)) {
        auto tok = std::move(head());
        TIRO_ASSERT(expected.contains(tok.type()), "Invalid token.");
        advance();
        return tok;
    }

    return {};
}

Parser::ResetLexerMode Parser::enter_lexer_mode(LexerMode mode) {
    LexerMode old = lexer_.mode();
    if (mode == old)
        return {nullptr, mode};

    lexer_.mode(mode);
    return {this, old};
}

} // namespace tiro
