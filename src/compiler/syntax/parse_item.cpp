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
        p.error_recover("Expected a function or a variable declaration", recovery);
    } else {
        p.error_recover("Expected a top level item.", recovery);
    }
    item.complete(SyntaxType::Error);
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
    m.complete(SyntaxType::ImportItem);
}

} // namespace tiro::next
