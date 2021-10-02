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

Compiler::Compiler(std::string_view module_name, const CompilerOptions& options)
    : options_(options) {
    TIRO_CHECK(!module_name.empty(), "module name must not be empty");
    module_name_ = strings_.insert(module_name);
}

void Compiler::add_file(std::string filename, std::string content) {
    if (started_)
        TIRO_ERROR("compiler already ran on this module");

    auto id = sources_.insert_new(std::move(filename), std::move(content));
    if (!id)
        TIRO_ERROR_WITH_CODE(TIRO_ERROR_BAD_ARG, "filename is not unique");
}

CompilerResult Compiler::run() {
    if (started_)
        TIRO_ERROR("compiler already ran on this module");
    started_ = true;

    CompilerResult result;
    if (!options_.parse) {
        result.success = true;
        return result;
    }

    auto ir_module = [&]() -> std::optional<ir::Module> {
        std::vector<SyntaxTreeEntry> files;
        files.reserve(sources_.size());
        for (auto file_id : sources_.ids())
            files.emplace_back(file_id, parse_file(sources_.content(file_id)));

        if (options_.keep_cst) {
            std::string buffer;
            for (const auto& entry : files) {
                buffer += fmt::format("# Filename: {}", sources_.filename(entry.id));
                buffer += dump(entry.tree, sources_.source_lines(entry.id));
            }
            result.cst = std::move(buffer);
        }

        auto ast = build_module_ast(files, strings_, diag_);
        if (!ast || !is_instance<AstModule>(ast))
            TIRO_ERROR("failed to build a module ast");

        if (options_.keep_ast) {
            result.ast = dump(ast.get(), strings_, sources_);
        }

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

CursorPosition Compiler::cursor_pos(const AbsoluteSourceRange& range) const {
    return sources_.cursor_pos(range.id(), range.range().begin());
}

SyntaxTree Compiler::parse_file(std::string_view source) {
    auto lex = [&]() {
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

    auto parse = [&](Span<const Token> tokens) {
        Parser parser(source, tokens);
        tiro::parse_file(parser);
        if (!parser.at(TokenType::Eof))
            parser.error("giving up before the end of file");

        return build_syntax_tree(source, parser.take_events());
    };

    if (auto res = validate_utf8(source); !res.ok) {
        SyntaxTree tree(source);
        tree.errors().push_back(SyntaxError(
            "The file contains invalid utf8.", SourceRange::from_std_offset(res.error_offset)));
        return tree;
    }

    auto tokens = lex();
    return parse(tokens);
}

std::optional<SemanticAst> Compiler::analyze(NotNull<AstModule*> root) {
    if (has_errors())
        return {};

    SemanticAst ast = analyze_ast(root, strings_, diag_);
    if (has_errors())
        return {};

    return ast;
}

std::optional<ir::Module> Compiler::generate_ir(const SemanticAst& ast) {
    TIRO_DEBUG_ASSERT(!has_errors(), "Must not generate mir when the program already has errors.");

    ir::Module mod(module_name_, strings_);
    ir::ModuleContext ctx{sources_, ast, diag_};
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
