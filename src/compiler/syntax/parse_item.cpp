#include "compiler/syntax/parse_item.hpp"

#include "compiler/syntax/parse_misc.hpp"
#include "compiler/syntax/parse_stmt.hpp"
#include "compiler/syntax/parser.hpp"
#include "compiler/syntax/token_set.hpp"

namespace tiro::next {

static TokenSet MODIFIERS = {
    TokenType::KwExport,
};

static std::optional<CompletedMarker> try_parse_modifiers(Parser& p);
static void parse_import(Parser& p);

void parse_item(Parser& p, const TokenSet& recovery) {
    auto modifiers = try_parse_modifiers(p);

    if (p.at(TokenType::KwImport) && !modifiers) {
        parse_import(p);
        return;
    }

    if (p.at_any(VAR_FIRST)) {
        parse_var_stmt(p, recovery.union_with(TokenType::Semicolon), modifiers);
        return;
    }

    if (p.at(TokenType::KwFunc)) {
        parse_func(p, recovery, modifiers);
        return;
    }

    auto m = modifiers ? modifiers->precede() : p.start();
    if (modifiers) {
        p.error_recover("Expected a function or a variable declaration", recovery);
    } else {
        p.error_recover("Expected a top level item.", recovery);
    }
    m.complete(SyntaxType::Error);
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
    while (!p.at(TokenType::Eof)) {
        p.expect(TokenType::Identifier);

        if (!p.accept(TokenType::Dot))
            break;
    }
    p.expect(TokenType::Semicolon);
    m.complete(SyntaxType::Import);
}

} // namespace tiro::next
