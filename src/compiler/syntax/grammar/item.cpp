#include "compiler/syntax/grammar/item.hpp"

#include "compiler/syntax/grammar/errors.hpp"
#include "compiler/syntax/grammar/misc.hpp"
#include "compiler/syntax/grammar/stmt.hpp"
#include "compiler/syntax/parser.hpp"
#include "compiler/syntax/token_set.hpp"

#include <absl/container/inlined_vector.h>

namespace tiro {

static const TokenSet MODIFIERS = {
    TokenType::KwExport,
};

static const TokenSet ITEM_FIRST = MODIFIERS //
                                       .union_with(VAR_FIRST)
                                       .union_with({
                                           TokenType::KwImport,
                                           TokenType::KwFunc,
                                       });

static const TokenSet CLOSING_BRACES = {
    TokenType::RightParen,
    TokenType::RightBracket,
    TokenType::RightBrace,
};

static std::optional<CompletedMarker> try_parse_modifiers(Parser& p);
static void parse_import(Parser& p);
static void parse_import_path(Parser& p, const TokenSet& recovery);

void parse_item(Parser& p, const TokenSet& recovery) {
    if (p.at(TokenType::KwImport)) {
        parse_import(p);
        return;
    }

    auto item = p.start();
    auto modifiers = try_parse_modifiers(p);
    if (p.at_any(VAR_FIRST)) {
        parse_var(p, recovery.union_with(TokenType::Semicolon), modifiers);
        p.expect(TokenType::Semicolon);
        item.complete(SyntaxType::VarItem);
        return;
    }

    if (p.at(TokenType::KwFunc)) {
        if (parse_func(p, recovery, modifiers) == FunctionKind::ShortExprBody)
            p.expect(TokenType::Semicolon);
        item.complete(SyntaxType::FuncItem);
        return;
    }

    if (modifiers) {
        p.error_recover("expected a function or a variable declaration", recovery);
    } else {
        p.error_recover("expected a top level item", recovery);
    }
    item.complete(SyntaxType::Error);
}

void parse_file(Parser& p) {
    auto m = p.start();

    while (!p.at(TokenType::Eof)) {
        if (p.accept(TokenType::Semicolon))
            continue;

        if (auto brace = p.accept_any(CLOSING_BRACES)) {
            auto err = p.start();
            p.advance();
            p.error("unmatched brace");
            err.complete(SyntaxType::Error);
            continue;
        }

        if (p.at_any(NESTING_START)) {
            discard_nested(p);
            continue;
        }

        parse_item(p, ITEM_FIRST);
    }

    m.complete(SyntaxType::File);
}

std::optional<CompletedMarker> try_parse_modifiers(Parser& p) {
    std::optional<Marker> modifiers;
    while (p.at_any(MODIFIERS)) {
        if (!modifiers)
            modifiers = p.start();

        p.advance();
    }
    if (modifiers)
        return modifiers->complete(SyntaxType::Modifiers);
    return {};
}

void parse_import(Parser& p) {
    TIRO_DEBUG_ASSERT(p.at(TokenType::KwImport), "Not at the start of an import item.");

    auto m = p.start();
    p.advance();

    parse_import_path(p, ITEM_FIRST.union_with({TokenType::KwAs, TokenType::Semicolon}));
    if (p.accept(TokenType::KwAs))
        p.expect(TokenType::Identifier);

    p.expect(TokenType::Semicolon);
    m.complete(SyntaxType::ImportItem);
}

void parse_import_path(Parser& p, const TokenSet& recovery) {
    auto m = p.start();

    while (!p.at(TokenType::Eof)) {
        if (!p.accept(TokenType::Identifier))
            p.error_recover("expected an import path segment", recovery.union_with(TokenType::Dot));

        if (!p.accept(TokenType::Dot))
            break;
    }

    m.complete(SyntaxType::ImportPath);
}

} // namespace tiro
