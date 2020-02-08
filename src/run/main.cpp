#include <fmt/format.h>

#include "tiro/compiler/compiler.hpp"
#include "tiro/core/scope.hpp"
#include "tiro/modules/modules.hpp"
#include "tiro/vm/context.hpp"
#include "tiro/vm/load.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace tiro;

template<typename... Args>
static void die(std::string_view message, Args&&... args) {
    std::cout << fmt::format(message, std::forward<Args>(args)...) << std::endl;
    std::exit(-1);
}

static void print_messages(
    const compiler::Compiler& compiler, const compiler::Diagnostics& diag) {
    for (auto& msg : diag.messages()) {
        if (msg.source) {
            auto pos = compiler.cursor_pos(msg.source);
            std::cout << "[" << pos.line() << ":" << pos.column() << "] ";
        }

        std::cout << msg.text << std::endl;
    }
}

static std::string read_file_contents(const char* path);

int main(int argc, char** argv) {
    const char* filename = nullptr;
    bool dump_ast = false;
    [[maybe_unused]] bool disassemble = false;
    std::string invoke;

    int positional = 0;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--disassemble") {
            disassemble = true;
        } else if (arg == "--dump-ast") {
            dump_ast = true;
        } else if (arg == "--invoke") {
            if (++i == argc) {
                die("Expected a function name after --invoke.");
            }
            invoke = argv[i];
        } else if (arg.size() >= 1 && arg[0] == '-') {
            die("Invalid option: {}", arg);
        } else if (positional == 0) {
            filename = argv[i];
            positional++;
        } else {
            die("Invalid positional argument: {}", arg);
        }
    }

    if (!filename) {
        die("Expected a filename.");
    }

    std::string content;

    try {
        content = read_file_contents(filename);
    } catch (const std::exception& e) {
        die("Failed to read the file: {}", e.what());
    }

    compiler::Compiler compiler(filename, content);
    const compiler::Diagnostics& diag = compiler.diag();

    compiler.parse();
    compiler.analyze();

    if (dump_ast) {
        std::cout << format_tree(compiler.ast_root(), compiler.strings())
                  << std::flush;
    }

    if (diag.has_errors()) {
        print_messages(compiler, diag);
        die("Aborting compilation ({} errors, {} warnings).",
            diag.error_count(), diag.warning_count());
    }

    std::unique_ptr<compiler::CompiledModule> module = compiler.codegen();
    if (diag.has_errors()) {
        print_messages(compiler, diag);
        die("Aborting compilation ({} errors, {} warnings).",
            diag.error_count(), diag.warning_count());
    }

    if (disassemble) {
        std::cout << disassemble_module(*module, compiler.strings())
                  << std::endl;
    }

    if (!invoke.empty()) {
        using namespace vm;

        Context ctx;
        {
            Root std(ctx, create_std_module(ctx));
            if (!ctx.add_module(std)) {
                TIRO_ERROR("Failed to register std module.");
            }

            Root io(ctx, create_io_module(ctx));
            if (!ctx.add_module(io)) {
                TIRO_ERROR("Failed to register io module.");
            }
        }

        Root<Module> mod(ctx, load_module(ctx, *module, compiler.strings()));
        Root<Function> func(ctx);
        {
            Tuple members = mod->members();
            const size_t size = members.size();
            for (size_t i = 0; i < size; ++i) {
                Value v = members.get(i);

                if (v.is<Function>()) {
                    Function f = v.as<Function>();
                    if (f.tmpl().name().view() == invoke) {
                        func.set(f);
                        break;
                    }
                }
            }
        }

        if (func->is_null()) {
            die("Failed to find function called {}.", invoke);
        }

        if (func->tmpl().params() != 0) {
            die("Function {} requires arguments.", invoke);
        }

        // TODO: Function arguments
        Root<Value> result(ctx, ctx.run(func.handle(), {}));
        std::cout << fmt::format("Function returned {} of type {}.",
                         to_string(result.get()), to_string(result->type()))
                  << std::endl;
    }
}

std::string read_file_contents(const char* path) {
    std::string result;

    FILE* file = std::fopen(path, "rb");
    if (!file) {
        throw std::system_error(errno, std::system_category(), "fopen");
    };
    ScopeExit close = [&] { std::fclose(file); };

    // Use the file size as a hint for the required memory (the file size might change while we're reading it).
    {
        auto size = std::filesystem::file_size(path);
        if (size > result.max_size()) {
            throw std::runtime_error("File is too large.");
        }
        result.reserve(size);
    }

    unsigned char buffer[4096];
    while (1) {
        size_t read = std::fread(buffer, 1, sizeof(buffer), file);
        if (ferror(file)) {
            throw std::system_error(errno, std::system_category(), "fread");
        }

        result.append(buffer, buffer + read);
        if (feof(file)) {
            break;
        }
    }

    return result;
}
