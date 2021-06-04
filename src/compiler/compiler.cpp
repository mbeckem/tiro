#include "compiler/compiler.hpp"

#include "bytecode/formatting.hpp"
#include "compiler/ast/ast.hpp"
#include "compiler/ast/dump.hpp"
#include "compiler/ast_gen/build_ast.hpp"
#include "compiler/bytecode_gen/module.hpp"
#include "compiler/ir/module.hpp"
#include "compiler/ir_gen/module.hpp"
#include "compiler/semantics/analysis.hpp"
#include "compiler/syntax/build_syntax_tree.hpp"
#include "compiler/syntax/dump.hpp"
#include "compiler/syntax/grammar/item.hpp"
#include "compiler/syntax/lexer.hpp"
#include "compiler/syntax/parser.hpp"

namespace tiro {

Compiler::Compiler(std::string file_name, std::string file_content, const CompilerOptions& options)
    : options_(options)
    , file_name_(std::move(file_name))
    , file_content_(std::move(file_content))
    , file_name_intern_(strings_.insert(file_name_))
    , source_map_(file_name_intern_, file_content_) {}

CompilerResult Compiler::run() {
    CompilerResult result;

    if (!options_.parse) {
        result.success = true;
        return result;
    }

    auto ir_module = [&]() -> std::optional<ir::Module> {
        auto file = parse_file();
        if (!file)
            return {};

        if (options_.keep_cst)
            result.cst = dump(*file, source_map_);

        auto ast = construct_ast(*file);
        if (options_.keep_ast)
            result.ast = dump(ast.get(), strings_, source_map_);

        if (!options_.analyze) {
            result.success = true;
            return {};
        }

        auto semantic_ast = analyze(TIRO_NN(ast.get()));
        if (!semantic_ast)
            return {};

        if (!options_.compile) {
            result.success = true;
            return {};
        }

        auto module = generate_ir(*semantic_ast);
        if (!module)
            return {};

        // Symbol table and ast no longer needed.
        return module;
    }();

    if (!ir_module)
        return result;

    if (options_.keep_ir) {
        StringFormatStream stream;
        dump_module(*ir_module, stream);
        result.ir = stream.take_str();
    }

    BytecodeModule module = generate_bytecode(*ir_module);
    if (options_.keep_bytecode) {
        StringFormatStream stream;
        format_module(module, stream);
        result.bytecode = stream.take_str();
    }

    result.success = true;
    result.module = std::make_unique<BytecodeModule>(std::move(module));
    return result;
}

CursorPosition Compiler::cursor_pos(const SourceRange& range) const {
    return source_map_.cursor_pos(range.begin());
}

std::optional<SyntaxTree> Compiler::parse_file() {
    auto lex = [&](std::string_view source) {
        Lexer lexer(source);
        lexer.ignore_comments(true);

        std::vector<Token> tokens;
        while (1) {
            auto token = lexer.next();
            bool is_eof = token.type() == TokenType::Eof;
            tokens.push_back(std::move(token));
            if (is_eof)
                break;
        }
        return tokens;
    };

    auto parse = [&](std::string_view source, Span<const Token> tokens) {
        Parser parser(source, tokens);
        tiro::parse_file(parser);
        if (!parser.at(TokenType::Eof))
            parser.error("giving up before the end of file");

        return build_syntax_tree(source, parser.take_events());
    };

    std::string_view source = file_content_;
    if (auto res = validate_utf8(source); !res.ok) {
        diag_.reportf(Diagnostics::Error, SourceRange::from_std_offset(res.error_offset),
            "The file contains invalid utf8.");
        return {};
    }

    auto tokens = lex(source);
    return parse(source, tokens);
}

AstPtr<AstFile> Compiler::construct_ast(const SyntaxTree& tree) {
    auto ast = build_file_ast(tree, strings_, diag_);
    TIRO_CHECK(ast, "Failed to construct the file's ast.");
    return ast;
}

std::optional<SemanticAst> Compiler::analyze(NotNull<AstFile*> root) {
    if (has_errors())
        return {};

    SemanticAst ast = analyze_ast(root, strings_, diag_);
    if (has_errors())
        return {};

    return ast;
}

std::optional<ir::Module> Compiler::generate_ir(const SemanticAst& ast) {
    TIRO_DEBUG_ASSERT(!has_errors(), "Must not generate mir when the program already has errors.");

    ir::Module mod(file_name_intern_, strings_);
    ir::ModuleContext ctx{file_content_, ast, diag_};
    ir::ModuleIRGen gen(ctx, mod);
    gen.compile_module();
    if (has_errors())
        return {};

    return std::optional(std::move(mod));
}

BytecodeModule Compiler::generate_bytecode(ir::Module& mod) {
    TIRO_DEBUG_ASSERT(
        !has_errors(), "Must not generate bytecode when the program already has errors.");
    return compile_module(mod);
}

} // namespace tiro
