#include "compiler/parser/parser.hpp"

#include "common/defs.hpp"
#include "common/math.hpp"
#include "compiler/parser/operators.hpp"

#include <fmt/format.h>

namespace tiro {

static std::string
unexpected_message(std::string_view context, TokenTypes expected, TokenType seen) {
    const size_t size = expected.size();

    fmt::memory_buffer buf;
    if (!context.empty()) {
        fmt::format_to(buf, "Unexpected {} in {} context", to_description(seen), context);
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

static const TokenTypes STRING_FIRST = {TokenType::SingleQuote, TokenType::DoubleQuote};

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

static const TokenTypes DECL_FIRST =
    TokenTypes{
        TokenType::KwImport,
        TokenType::KwFunc,
    }
        .union_with(VAR_DECL_FIRST);

static const TokenTypes STMT_FIRST =
    TokenTypes{
        TokenType::Semicolon,
        TokenType::KwAssert,
        TokenType::KwWhile,
        TokenType::KwFor,
    }
        .union_with(VAR_DECL_FIRST)
        .union_with(EXPR_FIRST);

static const TokenTypes MODIFIER_FIRST = {
    TokenType::KwExport,
};

static const TokenTypes TOPLEVEL_ITEM_FIRST = {
    TokenType::KwExport,
    TokenType::KwImport,
    TokenType::KwFunc,
    TokenType::Semicolon,
};

static const TokenTypes EXPR_STMT_OPTIONAL_SEMICOLON = {
    TokenType::KwFunc,
    TokenType::KwIf,
    TokenType::LeftBrace,
};

static bool can_begin_var_decl(TokenType type) {
    return VAR_DECL_FIRST.contains(type);
}

static bool can_begin_decl(TokenType type) {
    return DECL_FIRST.contains(type);
}

static bool can_begin_expression(TokenType type) {
    return EXPR_FIRST.contains(type);
}

static bool can_begin_modifier(TokenType type) {
    return MODIFIER_FIRST.contains(type);
}

static bool can_begin_string(TokenType type) {
    return STRING_FIRST.contains(type);
}

AstIdGenerator::AstIdGenerator()
    : next_id_(1) {}

AstId AstIdGenerator::generate() {
    if (TIRO_UNLIKELY(next_id_ == 0))
        TIRO_ERROR("Generated too many ast nodes.");

    return AstId(next_id_++);
}

bool Parser::parse_braced_list(
    const ListOptions& options, TokenTypes sync, FunctionRef<bool(TokenTypes inner_sync)> parser) {
    TIRO_DEBUG_ASSERT(!options.name.empty(), "Must not have an empty name.");
    TIRO_DEBUG_ASSERT(
        options.right_brace != TokenType::InvalidToken, "Must set the right brace token type.");
    TIRO_DEBUG_ASSERT(options.max_count == -1 || options.max_count >= 0, "Invalid max count.");

    int current_count = 0;

    if (accept(options.right_brace))
        return true;

    auto inner_sync = sync.union_with({TokenType::Comma, options.right_brace});

    while (1) {
        {
            const Token& current = head();
            if (current.type() == TokenType::Eof) {
                diag_.reportf(Diagnostics::Error, current.source(), "Unterminated {}, expected {}.",
                    options.name, to_description(options.right_brace));
                return false;
            }

            if (options.max_count != -1 && current_count >= options.max_count) {
                // TODO: Proper recovery until "," or brace?
                diag_.reportf(Diagnostics::Error, current.source(),
                    "Unexpected {} in {}, expected {}.", to_description(current.type()),
                    options.name, to_description(options.right_brace));
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
            if (!(next = recover_consume({TokenType::Comma, options.right_brace}, sync)))
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

template<typename Node, typename... Args>
AstPtr<Node> make_node(Args&&... args) {
    return std::make_unique<Node>(std::forward<Args>(args)...);
}

template<typename Parse, typename Recover>
auto Parser::parse_with_recovery(Parse&& parse, Recover&& recover) {
    auto result = parse();

    using result_type = decltype(result);

    if (!result && recover()) {
        auto node = result.take_node();
        if (node)
            return result_type(std::move(node));
        return result_type(syntax_error());
    }
    return result;
}

Parser::Parser(
    std::string_view file_name, std::string_view source, StringTable& strings, Diagnostics& diag)
    : file_name_(strings.insert(file_name))
    , source_(source)
    , strings_(strings)
    , diag_(diag)
    , lexer_(file_name_, source_, strings_, diag_) {

    advance();
}

Parser::Result<AstFile> Parser::parse_file() {
    auto start = mark_position();
    auto file = make_node<AstFile>();

    auto& items = file->items();
    while (!accept(TokenType::Eof)) {
        if (auto brace = accept(
                {TokenType::RightBrace, TokenType::RightBracket, TokenType::RightParen})) {
            diag_.reportf(Diagnostics::Error, brace->source(), "Unbalanced {}.",
                to_description(brace->type()));
            continue;
        }

        auto item = parse_item({});
        if (auto node = item.take_node())
            items.append(std::move(node));

        if (item.is_error() && !recover_seek(TOPLEVEL_ITEM_FIRST, {}))
            return partial(std::move(file), start);
    }

    return complete(std::move(file), start);
}

Parser::Result<AstStmt> Parser::parse_item(TokenTypes sync) {
    auto start = mark_position();
    auto start_token = head();
    auto type = start_token.type();

    if (type == TokenType::Semicolon) {
        auto empty = make_node<AstEmptyStmt>();
        advance();
        return complete(std::move(empty), start);
    }

    if (can_begin_modifier(type) || can_begin_decl(type)) {
        auto stmt = make_node<AstDeclStmt>();

        // TODO: Modifiers should be parsed by the parse_decl function?
        auto modifiers = parse_modifiers(sync);
        if (!modifiers)
            return partial(std::move(stmt), start);

        auto decl = [&]() -> Result<AstDecl> {
            auto decl_type = head().type();

            if (can_begin_var_decl(decl_type))
                return parse_var_decl(true, sync);

            if (decl_type == TokenType::KwFunc)
                return parse_func_decl(true, sync);

            if (decl_type == TokenType::KwImport)
                return parse_import_decl(sync);

            TIRO_UNREACHABLE("Unhandled declaration type in toplevel context.");
        }();

        stmt->decl(decl.take_node());
        if (!decl)
            return partial(std::move(stmt), start);

        if (stmt->decl())
            stmt->decl()->modifiers() = std::move(*modifiers.take_node());

        return complete(std::move(stmt), start);
    }

    diag_.reportf(Diagnostics::Error, start_token.source(), "Unexpected {}.", to_description(type));
    return syntax_error();
}

Parser::Result<AstNodeList<AstModifier>> Parser::parse_modifiers(TokenTypes sync) {
    AstNodeList<AstModifier> mods;

    while (can_begin_modifier(head().type())) {
        auto mod = parse_modifier(sync);
        if (mod.has_node())
            mods.append(mod.take_node());

        // TODO: Result should not require unique pointers. Implement builder for complex construction.
        if (!mod)
            return syntax_error(std::make_unique<AstNodeList<AstModifier>>(std::move(mods)));
    }

    return parse_success(std::make_unique<AstNodeList<AstModifier>>(std::move(mods)));
}

Parser::Result<AstModifier> Parser::parse_modifier([[maybe_unused]] TokenTypes sync) {
    auto start = mark_position();
    auto token = head();

    switch (token.type()) {
    case TokenType::KwExport: {
        auto exp = make_node<AstExportModifier>();
        advance();
        return complete(std::move(exp), start);
    }
    default:
        diag_.reportf(Diagnostics::Error, token.source(),
            "Expected a valid modifier but saw a {} instead.", to_description(token.type()));
        return syntax_error();
    }
}

Parser::Result<AstImportDecl> Parser::parse_import_decl(TokenTypes sync) {
    auto start_pos = mark_position();
    auto start_tok = expect(TokenType::KwImport);
    if (!start_tok)
        return syntax_error();

    auto parse = [&]() -> Result<AstImportDecl> {
        auto item = make_node<AstImportDecl>();

        std::vector<InternedString> path;
        auto path_ok = [&]() {
            while (1) {
                auto ident = expect(TokenType::Identifier);
                if (!ident)
                    return false;

                path.push_back(ident->data().as_string());
                if (ident->has_error())
                    return false;

                if (!accept(TokenType::Dot))
                    return true;

                // Else: continue with identifier after dot.
            };
        }();

        if (!path.empty())
            item->name(path.back());

        item->path(std::move(path));
        if (!path_ok)
            return partial(std::move(item), start_pos);

        if (!expect(TokenType::Semicolon))
            return partial(std::move(item), start_pos);

        return complete(std::move(item), start_pos);
    };

    return parse_with_recovery(
        parse, [&]() { return recover_consume(TokenType::Semicolon, sync); });
}

Parser::Result<AstFuncDecl> Parser::parse_func_decl(bool requires_name, TokenTypes sync) {
    auto start = mark_position();

    if (!expect(TokenType::KwFunc))
        return syntax_error();

    auto func = make_node<AstFuncDecl>();
    if (auto ident = accept(TokenType::Identifier)) {
        // TODO: Identifier node?
        func->name(ident->data().as_string());
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

    if (!expect(TokenType::LeftParen))
        return partial(std::move(func), start);

    auto& params = func->params();

    static constexpr ListOptions options{"parameter list", TokenType::RightParen};

    const bool params_ok = parse_braced_list(
        options, sync, [&]([[maybe_unused]] TokenTypes inner_sync) {
            auto param_ident = expect(TokenType::Identifier);
            if (!param_ident)
                return false;

            // TODO: Identifier node?
            auto param = make_node<AstParamDecl>();
            param->name(param_ident->data().as_string());
            params.append(
                complete_node(std::move(param), param_ident->source(), !param_ident->has_error()));
            return true;
        });
    if (!params_ok)
        return partial(std::move(func), start);

    if (auto eq = accept(TokenType::Equals))
        func->body_is_value(true);

    auto body = parse_block_expr(sync);
    func->body(body.take_node());
    if (!body)
        return partial(std::move(func), start);

    return complete(std::move(func), start);
}

Parser::Result<AstVarDecl> Parser::parse_var_decl(bool with_semicolon, TokenTypes sync) {
    auto parse = [&]() -> Result<AstVarDecl> {
        auto decl_start = mark_position();
        auto decl_tok = expect(VAR_DECL_FIRST);
        if (!decl_tok)
            return syntax_error();

        const bool is_const = decl_tok->type() == TokenType::KwConst;

        auto decl = make_node<AstVarDecl>();
        auto& bindings = decl->bindings();

        while (1) {
            auto binding = parse_binding(is_const, sync);
            bindings.append(binding.take_node());
            if (!binding)
                return partial(std::move(decl), decl_start);

            if (!accept(TokenType::Comma))
                break;
        }

        if (with_semicolon && !expect(TokenType::Semicolon))
            return partial(std::move(decl), decl_start);

        return complete(std::move(decl), decl_start);
    };

    auto recover = [&]() { return with_semicolon && recover_consume(TokenType::Semicolon, sync); };

    return parse_with_recovery(parse, recover);
}

Parser::Result<AstBinding> Parser::parse_binding(bool is_const, TokenTypes sync) {
    auto start = mark_position();
    auto binding = make_node<AstBinding>(is_const);

    auto spec = parse_binding_spec(sync);
    binding->spec(spec.take_node());
    if (!spec)
        return partial(std::move(binding), start);

    if (!accept(TokenType::Equals))
        return complete(std::move(binding), start);

    auto init = parse_expr(sync);
    binding->init(init.take_node());
    if (!init)
        return partial(std::move(binding), start);

    return complete(std::move(binding), start);
}

Parser::Result<AstBindingSpec> Parser::parse_binding_spec(TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = head();

    switch (start_tok.type()) {
    case TokenType::LeftParen: {
        advance();

        static constexpr auto options =
            ListOptions("tuple declaration", TokenType::RightParen).set_allow_trailing_comma(true);

        auto spec = make_node<AstTupleBindingSpec>();

        auto& names = spec->names();
        const bool list_ok = parse_braced_list(
            options, sync, [&]([[maybe_unused]] TokenTypes inner_sync) {
                auto ident = parse_string_identifier(inner_sync);
                if (ident.has_node())
                    names.append(ident.take_node());

                return !ident.is_error();
            });

        if (!list_ok)
            return partial(std::move(spec), start);

        if (names.size() == 0) {
            spec->has_error(true);
            diag_.report(Diagnostics::Error, start_tok.source(),
                "Variable lists must not be empty in tuple unpacking "
                "declarations.");
            // Parser is still ok - just report the grammar error
        }

        return complete(std::move(spec), start);
    }

    case TokenType::Identifier: {
        auto spec = make_node<AstVarBindingSpec>();

        auto ident = parse_string_identifier(sync);
        spec->name(ident.take_node());
        if (!ident)
            return partial(std::move(spec), start);

        return complete(std::move(spec), start);
    }

    default:
        diag_.reportf(Diagnostics::Error, start_tok.source(),
            "Unexpected {}, expected a valid identifier or a '('.",
            to_description(start_tok.type()));
        return syntax_error();
    }
}

Parser::Result<AstStmt> Parser::parse_stmt(TokenTypes sync) {
    auto start = mark_position();

    // FIXME: Semicolon recovery for all rules?

    if (auto empty_tok = accept(TokenType::Semicolon)) {
        auto stmt = make_node<AstEmptyStmt>();
        return complete(std::move(stmt), start);
    }

    const TokenType type = head().type();

    if (type == TokenType::KwAssert)
        return parse_assert_stmt(sync);

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

    if (can_begin_var_decl(type))
        return parse_var_stmt(sync);

    if (can_begin_expression(type))
        return parse_expr_stmt(sync);

    // Hint: can_begin_expression could be out of sync with
    // the expression parser.
    diag_.reportf(Diagnostics::Error, head().source(), "Unexpected {} in statement context.",
        to_description(type));
    return syntax_error();
}

Parser::Result<AstAssertStmt> Parser::parse_assert_stmt(TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = expect(TokenType::KwAssert);
    if (!start_tok)
        return syntax_error();

    auto parse = [&]() -> Result<AstAssertStmt> {
        auto stmt = make_node<AstAssertStmt>();

        if (!expect(TokenType::LeftParen))
            return partial(std::move(stmt), start);

        // TODO min args?
        static constexpr auto options =
            ListOptions("assertion statement", TokenType::RightParen).set_max_count(2);

        int argument = 0;
        const bool args_ok = parse_braced_list(options, sync, [&](TokenTypes inner_sync) mutable {
            switch (argument++) {
            // Condition
            case 0: {
                auto expr = parse_expr(inner_sync);
                stmt->cond(expr.take_node());
                return expr.is_ok();
            }

            // Optional message
            case 1: {
                auto expr = parse_expr(inner_sync);
                if (auto node = expr.take_node()) {
                    if (auto message = try_cast<AstStringExpr>(node)) {
                        stmt->message(std::move(message));
                    } else {
                        diag_.reportf(Diagnostics::Error, node->source(),
                            "Expected a string literal.", to_string(node->type()));
                        // Continue parsing, this is ok ..
                    }
                }
                return expr.is_ok();
            }
            default:
                TIRO_UNREACHABLE("Assertion argument parser called too often.");
            }
        });

        if (argument < 1) {
            diag_.report(Diagnostics::Error, start_tok->source(),
                "Assertion must have at least one argument.");
            stmt->has_error(true);
        }

        if (!args_ok)
            return partial(std::move(stmt), start);

        if (!expect(TokenType::Semicolon))
            return partial(std::move(stmt), start);

        return complete(std::move(stmt), start);
    };

    return parse_with_recovery(
        parse, [&]() { return recover_consume(TokenType::Semicolon, sync); });
}

Parser::Result<AstWhileStmt> Parser::parse_while_stmt(TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = expect(TokenType::KwWhile);
    if (!start_tok)
        return syntax_error();

    auto stmt = make_node<AstWhileStmt>();

    auto cond = parse_expr(sync.union_with(TokenType::LeftBrace));
    stmt->cond(cond.take_node());
    if (!cond)
        stmt->has_error(true);

    if (head().type() != TokenType::LeftBrace) {
        recover_seek(TokenType::LeftBrace, sync);
        stmt->has_error(true);
    }

    auto body = parse_block_expr(sync);
    stmt->body(body.take_node());
    if (!body)
        return partial(std::move(stmt), start);

    return complete(std::move(stmt), start);
}

Parser::Result<AstForStmt> Parser::parse_for_stmt(TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = expect(TokenType::KwFor);
    if (!start_tok)
        return syntax_error();

    auto stmt = make_node<AstForStmt>();

    if (!parse_for_stmt_header(stmt.get(), sync))
        return partial(std::move(stmt), start);

    auto body = parse_block_expr(sync);
    stmt->body(body.take_node());
    if (!body)
        return partial(std::move(stmt), start);

    return complete(std::move(stmt), start);
}

bool Parser::parse_for_stmt_header(AstForStmt* stmt, TokenTypes sync) {
    const bool has_parens = static_cast<bool>(accept(TokenType::LeftParen));

    auto parse_init = [&] {
        auto parse = [&]() -> Result<AstVarDecl> {
            if (const Token& tok = head(); !can_begin_var_decl(tok.type())) {
                diag_.reportf(Diagnostics::Error, tok.source(),
                    "Expected a variable declaration or a {}.",
                    to_description(TokenType::Semicolon));
                return syntax_error();
            }

            auto decl = parse_var_decl(false, sync.union_with(TokenType::Semicolon));
            if (!decl)
                return decl;

            if (!expect(TokenType::Semicolon))
                return syntax_error(decl.take_node());

            return decl;
        };

        auto recover = [&] { return recover_consume(TokenType::Semicolon, sync).has_value(); };

        return parse_with_recovery(parse, recover);
    };

    auto parse_condition = [&] {
        auto parse = [&]() -> Result<AstExpr> {
            auto expr = parse_expr(sync.union_with(TokenType::Semicolon));
            if (!expr)
                return expr;

            if (!expect(TokenType::Semicolon))
                return syntax_error(expr.take_node());

            return expr;
        };

        auto recover = [&] { return recover_consume(TokenType::Semicolon, sync).has_value(); };

        return parse_with_recovery(parse, recover);
    };

    auto parse_step = [&](TokenType next) {
        auto parse = [&]() -> Result<AstExpr> { return parse_expr(sync.union_with(next)); };

        auto recover = [&]() { return recover_seek(next, sync); };

        return parse_with_recovery(parse, recover);
    };

    auto parse = [&] {
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
            stmt->cond(cond.take_node());
            if (!cond)
                return false;
        }

        // Optional step expression
        const TokenType next = has_parens ? TokenType::RightParen : TokenType::LeftBrace;
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

    auto recover = [&] {
        return has_parens ? recover_consume(TokenType::RightParen, sync).has_value()
                          : recover_seek(TokenType::LeftBrace, sync);
    };

    if (!parse()) {
        stmt->has_error(true);
        return recover();
    }
    return true;
}

// TODO: Unify with parse_items implementation for decl statements (see usage of this function).
Parser::Result<AstDeclStmt> Parser::parse_var_stmt(TokenTypes sync) {
    auto start = mark_position();
    auto stmt = make_node<AstDeclStmt>();

    auto decl = parse_var_decl(true, sync);
    stmt->decl(decl.take_node());
    if (!decl)
        return partial(std::move(stmt), start);

    return complete(std::move(stmt), start);
}

Parser::Result<AstExprStmt> Parser::parse_expr_stmt(TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = head();

    const bool need_semicolon = !EXPR_STMT_OPTIONAL_SEMICOLON.contains(start_tok.type());

    auto parse = [&]() -> Result<AstExprStmt> {
        auto stmt = make_node<AstExprStmt>();

        auto expr = parse_expr(sync.union_with(TokenType::Semicolon));
        stmt->expr(expr.take_node());
        if (!expr)
            return partial(std::move(stmt), start);

        if (need_semicolon) {
            if (!expect(TokenType::Semicolon))
                return partial(std::move(stmt), start);
        } else {
            accept(TokenType::Semicolon);
        }
        return complete(std::move(stmt), start);
    };

    return parse_with_recovery(
        parse, [&]() { return recover_consume(TokenType::Semicolon, sync); });
}

Parser::Result<AstExpr> Parser::parse_expr(TokenTypes sync) {
    return parse_expr(0, sync);
}

/// Recursive function that implements a pratt parser.
///
/// See also:
///      http://crockford.com/javascript/tdop/tdop.html
///      https://www.oilshell.org/blog/2016/11/01.html
///      https://groups.google.com/forum/#!topic/comp.compilers/ruJLlQTVJ8o
Parser::Result<AstExpr> Parser::parse_expr(int min_precedence, TokenTypes sync) {
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

Parser::Result<AstExpr>
Parser::parse_infix_expr(AstPtr<AstExpr> left, int current_precedence, TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = head();

    if (auto op = to_binary_operator(start_tok.type())) {
        auto binary_expr = make_node<AstBinaryExpr>(*op);
        advance();
        binary_expr->left(std::move(left));

        int next_precedence = current_precedence;
        if (!operator_is_right_associative(*op))
            ++next_precedence;

        auto right = parse_expr(next_precedence, sync);
        binary_expr->right(right.take_node());
        if (!right)
            return partial(std::move(binary_expr), start);

        return complete(std::move(binary_expr), start);
    }

    switch (start_tok.type()) {
    case TokenType::LeftParen:
    case TokenType::QuestionLeftParen:
        return parse_call_expr(std::move(left), sync);
    case TokenType::LeftBracket:
    case TokenType::QuestionLeftBracket:
        return parse_index_expr(std::move(left), sync);
    case TokenType::Dot:
    case TokenType::QuestionDot:
        return parse_member_expr(std::move(left), sync);
    default:
        TIRO_ERROR(
            "Invalid operator in parse_infix_operator: {}", to_description(start_tok.type()));
    }
}

/// Parses a unary expressions. Unary expressions are either plain primary
/// expressions or a unary operator followed by another unary expression.
Parser::Result<AstExpr> Parser::parse_prefix_expr(TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = head();

    auto op = to_unary_operator(start_tok.type());
    if (!op)
        return parse_primary_expr(sync);

    auto unary = make_node<AstUnaryExpr>(*op);
    advance();

    auto inner = parse_expr(unary_precedence, sync);
    unary->inner(inner.take_node());
    return forward(std::move(unary), start, inner);
}

Parser::Result<AstExpr>
Parser::parse_member_expr(AstPtr<AstExpr> current, [[maybe_unused]] TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = expect({TokenType::Dot, TokenType::QuestionDot});
    if (!start_tok)
        return syntax_error();

    auto access_type = start_tok->type() == TokenType::Dot ? AccessType::Normal
                                                           : AccessType::Optional;
    auto expr = make_node<AstPropertyExpr>(AccessType::Normal);
    expr->access_type(access_type);
    expr->instance(std::move(current));

    auto property = parse_property_identifier(sync);
    expr->property(property.take_node());
    if (!property)
        return partial(std::move(expr), start);

    return complete(std::move(expr), start);
}

Parser::Result<AstExpr> Parser::parse_call_expr(AstPtr<AstExpr> current, TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = expect({TokenType::LeftParen, TokenType::QuestionLeftParen});
    if (!start_tok)
        return syntax_error();

    auto access_type = start_tok->type() == TokenType::LeftParen ? AccessType::Normal
                                                                 : AccessType::Optional;

    auto call = make_node<AstCallExpr>(AccessType::Normal);
    call->access_type(access_type);
    call->func(std::move(current));

    static constexpr ListOptions options{"argument list", TokenType::RightParen};

    auto& args = call->args();
    bool list_ok = parse_braced_list(options, sync, [&](TokenTypes inner_sync) {
        auto arg = parse_expr(inner_sync);
        if (arg.has_node())
            args.append(arg.take_node());

        return arg.is_ok();
    });

    if (!list_ok)
        return partial(std::move(call), start);

    return complete(std::move(call), start);
}

Parser::Result<AstExpr> Parser::parse_index_expr(AstPtr<AstExpr> current, TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = expect({TokenType::LeftBracket, TokenType::QuestionLeftBracket});
    if (!start_tok)
        return syntax_error();

    auto access_type = start_tok->type() == TokenType::LeftBracket ? AccessType::Normal
                                                                   : AccessType::Optional;

    auto expr = make_node<AstElementExpr>(AccessType::Normal);
    expr->access_type(access_type);
    expr->instance(std::move(current));

    auto parse = [&]() -> Result<AstElementExpr> {
        auto element = parse_expr(TokenType::RightBracket);
        expr->element(element.take_node());
        if (!element)
            return partial(std::move(expr), start);

        if (!expect(TokenType::RightBracket))
            return partial(std::move(expr), start);

        return complete(std::move(expr), start);
    };

    return parse_with_recovery(
        parse, [&]() { return recover_consume(TokenType::RightBracket, sync); });
}

Parser::Result<AstExpr> Parser::parse_primary_expr(TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = head();

    if (can_begin_string(start_tok.type()))
        return parse_string_group(sync);

    switch (start_tok.type()) {
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
        auto ret = make_node<AstReturnExpr>();
        advance();

        if (can_begin_expression(head().type())) {
            auto value = parse_expr(sync);
            ret->value(value.take_node());
            if (!value)
                return partial(std::move(ret), start);
        }
        return complete(std::move(ret), start);
    }

    // Continue expression
    case TokenType::KwContinue: {
        auto cont = make_node<AstContinueExpr>();
        advance();
        return complete(std::move(cont), start);
    }

    // Break expression
    case TokenType::KwBreak: {
        auto brk = make_node<AstBreakExpr>();
        advance();
        return complete(std::move(brk), start);
    }

    // Variable reference
    case TokenType::Identifier: {
        return parse_var_expr(sync);
    }

    // Function Literal
    case TokenType::KwFunc: {
        auto ret = make_node<AstFuncExpr>();

        auto decl = parse_func_decl(false, sync);
        ret->decl(decl.take_node());
        if (!decl)
            return partial(std::move(ret), start);

        return complete(std::move(ret), start);
    }

    // Array literal.
    case TokenType::LeftBracket: {
        auto lit = make_node<AstArrayLiteral>();
        advance();

        static constexpr auto options =
            ListOptions("array literal", TokenType::RightBracket).set_allow_trailing_comma(true);

        auto& items = lit->items();
        bool list_ok = parse_braced_list(options, sync, [&](TokenTypes inner_sync) {
            auto value = parse_expr(inner_sync);
            if (value.has_node())
                items.append(value.take_node());

            return value.is_ok();
        });

        if (!list_ok)
            return partial(std::move(lit), start);

        return complete(std::move(lit), start);
    }

    // Map literal
    case TokenType::KwMap: {
        auto lit = make_node<AstMapLiteral>();
        advance();

        auto entries_start = expect(TokenType::LeftBrace);
        if (!entries_start)
            return partial(std::move(lit), start);

        static constexpr auto options =
            ListOptions("map literal", TokenType::RightBrace).set_allow_trailing_comma(true);

        auto parse_item = [&](TokenTypes entry_sync) -> Result<AstMapItem> {
            auto item_start = mark_position();
            auto item = make_node<AstMapItem>();

            auto key = parse_expr(entry_sync.union_with(TokenType::Colon));
            item->key(key.take_node());
            if (!key)
                return partial(std::move(item), item_start);

            if (!expect(TokenType::Colon))
                return partial(std::move(item), item_start);

            auto value = parse_expr(entry_sync);
            item->value(value.take_node());
            if (!value)
                return partial(std::move(item), item_start);

            return complete(std::move(item), item_start);
        };

        auto& items = lit->items();
        const bool list_ok = parse_braced_list(options, sync, [&](TokenTypes inner_sync) {
            auto item = parse_item(inner_sync);
            if (item.has_node())
                items.append(item.take_node());

            return item.is_ok();
        });

        if (!list_ok)
            return partial(std::move(lit), start);

        return complete(std::move(lit), start);
    }

    // Set literal
    case TokenType::KwSet: {
        auto lit = make_node<AstSetLiteral>();
        advance();

        auto entries_start = expect(TokenType::LeftBrace);
        if (!entries_start)
            return partial(std::move(lit), start);

        static constexpr auto options =
            ListOptions("set literal", TokenType::RightBrace).set_allow_trailing_comma(true);

        auto& items = lit->items();
        const bool list_ok = parse_braced_list(options, sync, [&](TokenTypes inner_sync) {
            auto value = parse_expr(inner_sync);
            if (value.has_node())
                items.append(value.take_node());

            return value.is_ok();
        });

        if (!list_ok)
            return partial(std::move(lit), start);

        return complete(std::move(lit), start);
    }

    // Null Literal
    case TokenType::KwNull: {
        auto lit = make_node<AstNullLiteral>();
        lit->has_error(start_tok.has_error());
        advance();
        return complete(std::move(lit), start);
    }

    // Boolean literals
    case TokenType::KwTrue:
    case TokenType::KwFalse: {
        auto lit = make_node<AstBooleanLiteral>(start_tok.type() == TokenType::KwTrue);
        lit->has_error(start_tok.has_error());
        advance();
        return complete(std::move(lit), start);
    }

    // Symbol literal
    case TokenType::SymbolLiteral: {
        auto sym = make_node<AstSymbolLiteral>(start_tok.data().as_string());
        sym->has_error(start_tok.has_error());
        advance();
        return complete(std::move(sym), start);
    }

    // Integer literal
    case TokenType::IntegerLiteral: {
        auto lit = make_node<AstIntegerLiteral>(start_tok.data().as_integer());
        lit->has_error(start_tok.has_error());
        advance();
        return complete(std::move(lit), start);
    }

    // Float literal
    case TokenType::FloatLiteral: {
        auto lit = make_node<AstFloatLiteral>(start_tok.data().as_float());
        lit->has_error(start_tok.has_error());
        advance();
        return complete(std::move(lit), start);
    }

    default:
        break;
    }

    diag_.reportf(Diagnostics::Error, start_tok.source(),
        "Unexpected {}, expected a valid expression.", to_description(start_tok.type()));
    return syntax_error();
}

Parser::Result<AstExpr> Parser::parse_var_expr([[maybe_unused]] TokenTypes sync) {
    auto start = mark_position();
    auto tok = expect(TokenType::Identifier);
    if (!tok)
        return syntax_error();

    auto expr = make_node<AstVarExpr>(tok->data().as_string());
    expr->has_error(tok->has_error());
    return complete(std::move(expr), start);
}

Parser::Result<AstExpr> Parser::parse_block_expr(TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = expect(TokenType::LeftBrace);
    if (!start_tok)
        return syntax_error();

    auto parse = [&]() -> Result<AstBlockExpr> {
        auto block = make_node<AstBlockExpr>();

        auto& stmts = block->stmts();
        while (!accept(TokenType::RightBrace)) {
            if (const Token& tok = head(); tok.type() == TokenType::Eof) {
                diag_.reportf(Diagnostics::Error, tok.source(),
                    "Unterminated block expression, expected {}.",
                    to_description(TokenType::RightBrace));
                return partial(std::move(block), start);
            }

            auto stmt = parse_stmt(sync.union_with(TokenType::RightBrace));
            if (stmt.has_node())
                stmts.append(stmt.take_node());

            if (!stmt)
                return partial(std::move(block), start);
        }

        return complete(std::move(block), start);
    };

    return parse_with_recovery(
        parse, [&]() { return recover_consume(TokenType::RightBrace, sync); });
}

Parser::Result<AstExpr> Parser::parse_if_expr(TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = expect(TokenType::KwIf);
    if (!start_tok)
        return syntax_error();

    auto expr = make_node<AstIfExpr>();

    auto cond = parse_expr(TokenType::LeftBrace);
    expr->cond(cond.take_node());
    if (!cond && !recover_seek(TokenType::LeftBrace, sync))
        return partial(std::move(expr), start);

    auto then_expr = parse_block_expr(sync.union_with(TokenType::KwElse));
    expr->then_branch(then_expr.take_node());
    if (!then_expr && !recover_seek(TokenType::KwElse, sync))
        return partial(std::move(expr), start);

    if (auto else_tok = accept(TokenType::KwElse)) {
        if (head().type() == TokenType::KwIf) {
            auto nested = parse_if_expr(sync);
            expr->else_branch(nested.take_node());
            if (!nested)
                return partial(std::move(expr), start);
        } else {
            auto else_expr = parse_block_expr(sync);
            expr->else_branch(else_expr.take_node());
            if (!else_expr)
                return partial(std::move(expr), start);
        }
    }

    return complete(std::move(expr), start);
}

Parser::Result<AstExpr> Parser::parse_paren_expr(TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = expect(TokenType::LeftParen);
    if (!start_tok)
        return syntax_error();

    auto parse = [&]() -> Result<AstExpr> {
        // "()" is the empty tuple.
        if (accept(TokenType::RightParen)) {
            auto tuple = make_node<AstTupleLiteral>();
            return complete(std::move(tuple), start);
        }

        // Parse the initial expression - we don't know whether this is a tuple yet.
        auto expr = parse_expr(sync.union_with({TokenType::Comma, TokenType::RightParen}));
        if (!expr)
            return expr;

        auto initial = expr.take_node();

        auto next = expect({TokenType::Comma, TokenType::RightParen});
        if (!next)
            return syntax_error(std::move(initial));

        // "(expr)" is a simple braced expression, not a tuple.
        if (next->type() == TokenType::RightParen)
            return initial;

        // "(expr, ..." is guaranteed to be a tuple.
        if (next->type() == TokenType::Comma)
            return parse_tuple(start, std::move(initial), sync);

        TIRO_UNREACHABLE("Invalid token type.");
    };

    return parse_with_recovery(
        parse, [&]() { return recover_consume(TokenType::RightParen, sync); });
}

Parser::Result<AstExpr>
Parser::parse_tuple(u32 start, AstPtr<AstExpr> first_item, TokenTypes sync) {
    auto tuple = make_node<AstTupleLiteral>();

    auto& items = tuple->items();
    if (first_item)
        items.append(std::move(first_item));

    static constexpr auto options =
        ListOptions("tuple literal", TokenType::RightParen).set_allow_trailing_comma(true);

    bool list_ok = parse_braced_list(options, sync, [&](TokenTypes inner_sync) {
        auto expr = parse_expr(inner_sync);
        if (expr.has_node())
            items.append(expr.take_node());
        return expr.is_ok();
    });

    if (!list_ok)
        return partial(std::move(tuple), start);

    return complete(std::move(tuple), start);
}

Parser::Result<AstExpr> Parser::parse_string_group(TokenTypes sync) {
    auto start = mark_position();

    auto str_result = parse_string_expr(sync);
    if (!str_result || !str_result.has_node())
        return str_result;

    auto str = str_result.take_node();

    // Adjacent string literals are grouped together in a sequence.
    if (can_begin_string(head().type())) {
        auto group = make_node<AstStringGroupExpr>();
        auto& strings = group->strings();
        strings.append(std::move(str));

        while (1) {
            auto next_str_result = parse_string_expr(sync);
            if (next_str_result.has_node())
                strings.append(next_str_result.take_node());
            if (!next_str_result)
                return partial(std::move(group), start);

            if (!can_begin_string(head().type()))
                break;
        }

        return complete(std::move(group), start);
    }

    return parse_success(std::move(str));
}

Parser::Result<AstStringExpr> Parser::parse_string_expr(TokenTypes sync) {
    auto start = mark_position();
    auto start_tok = expect({TokenType::SingleQuote, TokenType::DoubleQuote});
    if (!start_tok)
        return syntax_error();

    auto end_type = start_tok->type();
    auto lexer_mode = start_tok->type() == TokenType::SingleQuote ? LexerMode::StringSingleQuote
                                                                  : LexerMode::StringDoubleQuote;
    auto mode_guard = enter_lexer_mode(lexer_mode);

    auto parse = [&]() -> Result<AstStringExpr> {
        auto expr = make_node<AstStringExpr>();
        auto& items = expr->items();

        while (1) {
            auto item_start = mark_position();
            auto item_tok = expect({TokenType::StringContent, TokenType::Dollar,
                TokenType::DollarLeftBrace, end_type});
            if (!item_tok)
                return partial(std::move(expr), start);

            if (item_tok->type() == end_type)
                break;

            if (item_tok->type() == TokenType::StringContent) {
                auto str = make_node<AstStringLiteral>(item_tok->data().as_string());

                items.append(complete_node(std::move(str), item_start, !item_tok->has_error()));
                if (item_tok->has_error())
                    return partial(std::move(expr), start);

                continue;
            }

            auto item_expr = parse_interpolated_expr(item_tok->type(), sync.union_with(end_type));
            if (item_expr.has_node())
                items.append(item_expr.take_node());
            if (!item_expr)
                return partial(std::move(expr), start);

            // Else: continue with next iteration, lexer mode is restored
        }

        return complete(std::move(expr), start);
    };

    return parse_with_recovery(parse, [&]() { return recover_consume(end_type, sync); });
}

Parser::Result<AstExpr> Parser::parse_interpolated_expr(TokenType starter, TokenTypes sync) {
    TIRO_DEBUG_ASSERT(starter == TokenType::Dollar || starter == TokenType::DollarLeftBrace,
        "Must start with $ or ${.");

    auto normal_mode = enter_lexer_mode(LexerMode::Normal);
    auto peek = head();

    if (starter == TokenType::Dollar) {
        if (peek.type() != TokenType::Identifier) {
            diag_.reportf(Diagnostics::Error, peek.source(),
                "Unexpected {}, expected an identifier. Use '${{' (no "
                "space) to include a complex expression or use '\\$' to escape "
                "the dollar sign.",
                to_description(peek.type()));
            return syntax_error();
        }

        return parse_var_expr(sync);
    }

    if (starter == TokenType::DollarLeftBrace) {
        auto parse = [&]() -> Result<AstExpr> {
            auto expr = parse_expr(sync.union_with(TokenType::RightBrace));
            if (!expr)
                return expr;

            if (!expect(TokenType::RightBrace))
                return syntax_error(expr.take_node());

            return expr;
        };

        return parse_with_recovery(
            parse, [&]() { return recover_consume(TokenType::RightBrace, sync); });
    }

    TIRO_UNREACHABLE("Invalid token type to start an interpolated expression.");
}

Parser::Result<AstIdentifier> Parser::parse_property_identifier([[maybe_unused]] TokenTypes sync) {
    auto mode_guard = enter_lexer_mode(LexerMode::Member);

    auto start = mark_position();
    auto member_tok = expect({TokenType::Identifier, TokenType::NumericMember});
    if (!member_tok)
        return syntax_error();

    switch (member_tok->type()) {
    case TokenType::Identifier: {
        auto ident = make_node<AstStringIdentifier>(member_tok->data().as_string());
        ident->value(member_tok->data().as_string());
        if (member_tok->has_error())
            ident->has_error(true);

        return complete(std::move(ident), start);
    }

    case TokenType::NumericMember: {
        auto ident = make_node<AstNumericIdentifier>(0);

        const i64 value = member_tok->data().as_integer();
        if (value < 0 || value > std::numeric_limits<u32>::max()) {
            diag_.reportf(Diagnostics::Error, member_tok->source(),
                "Integer value {} cannot be used as a tuple member index.", value);
            ident->has_error(true);
        } else {
            ident->value(static_cast<u32>(value));
        }

        if (member_tok->has_error())
            ident->has_error(true);

        return complete(std::move(ident), start);
    }

    default:
        TIRO_UNREACHABLE("Invalid token type.");
    }
}

Parser::Result<AstStringIdentifier>
Parser::parse_string_identifier([[maybe_unused]] TokenTypes sync) {
    auto start = mark_position();
    auto token = expect(TokenType::Identifier);
    if (!token)
        return syntax_error();

    auto ident = make_node<AstStringIdentifier>(token->data().as_string());
    ident->value(token->data().as_string());
    if (token->has_error())
        ident->has_error(true);

    return complete(std::move(ident), start);
}

void Parser::complete_node(AstNode* node, u32 start, bool success) {
    TIRO_DEBUG_ASSERT(node, "Node must not be null.");

    complete_node(node, ref(start, last_ ? last_->source().end() : start), success);
}

void Parser::complete_node(AstNode* node, const SourceReference& source, bool success) {
    TIRO_DEBUG_ASSERT(node, "Node must not be null.");

    node->id(node_ids_.generate());
    node->source(source);
    if (!success)
        node->has_error(true);
}

Token& Parser::head() {
    if (!head_) {
        head_ = lexer_.next();
    }
    return *head_;
}

void Parser::advance() {
    last_ = std::move(head_);
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
    TIRO_DEBUG_ASSERT(!tokens.empty(), "Token set must not be empty.");

    auto res = accept(tokens);
    if (!res) {
        const Token& tok = head();
        diag_.report(Diagnostics::Error, tok.source(), unexpected_message({}, tokens, tok.type()));
    }
    return res;
}

bool Parser::recover_seek(TokenTypes expected, TokenTypes sync) {
    // TODO: It might be useful to track opening / closing braces in here?
    // We might be skipping over them otherwise.
    while (1) {
        const Token& tok = head();

        if (tok.type() == TokenType::Eof || tok.type() == TokenType::InvalidToken)
            return false;

        if (expected.contains(tok.type()))
            return true;

        if (sync.contains(tok.type()))
            return false;

        advance();
    }
}

std::optional<Token> Parser::recover_consume(TokenTypes expected, TokenTypes sync) {
    if (recover_seek(expected, sync)) {
        auto tok = std::move(head());
        TIRO_DEBUG_ASSERT(expected.contains(tok.type()), "Invalid token.");
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

u32 Parser::mark_position() {
    return head().source().begin();
}

} // namespace tiro
