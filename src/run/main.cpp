#include <fmt/format.h>

#include "bytecode/module.hpp"
#include "common/scope_guards.hpp"
#include "compiler/compiler.hpp"
#include "vm/context.hpp"
#include "vm/module_registry.hpp"
#include "vm/modules/modules.hpp"

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

static void print_messages(const Compiler& compiler, const Diagnostics& diag) {
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
    try {
        const char* filename = nullptr;
        bool dump_ast = false;
        bool disassemble = false;
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

        CompilerOptions options;
        options.keep_ast = dump_ast;
        options.keep_bytecode = disassemble;

        Compiler compiler(filename, content, options);
        const Diagnostics& diag = compiler.diag();
        auto compiler_result = compiler.run();

        if (dump_ast && compiler_result.ast) {
            std::cout << *compiler_result.ast << std::endl;
        }

        if (disassemble && compiler_result.bytecode) {
            std::cout << *compiler_result.bytecode << std::endl;
        }

        if (diag.has_errors()) {
            print_messages(compiler, diag);
            die("Aborting compilation ({} errors, {} warnings).", diag.error_count(),
                diag.warning_count());
        }

        auto& module = compiler_result.module;
        TIRO_CHECK(module, "Must have compiled a valid bytecode module.");

        if (!invoke.empty()) {
            using namespace vm;

            vm::Context ctx;
            vm::Scope sc(ctx);
            {
                vm::Local std = sc.local(create_std_module(ctx));
                if (!ctx.modules().add_module(ctx, std)) {
                    TIRO_ERROR("Failed to register std module.");
                }
            }

            vm::Local mod = sc.local(load_module(ctx, *module));
            vm::Local func = sc.local();
            {
                vm::Local vm_name = sc.local(ctx.get_symbol(invoke));
                if (auto found = mod->find_exported(*vm_name)) {
                    func.set(*found);
                }
            }

            if (func->is_null()) {
                die("Failed to find function called {}.", invoke);
            }

            // TODO: Function arguments
            // TODO: ASYNC
            vm::Local result = sc.local(ctx.run_init(func, {}));
            std::cout << fmt::format(
                "Function returned {} of type {}.", to_string(*result), to_string(result->type()))
                      << std::endl;
        }
    } catch (const std::exception& e) {
        die("Error: {}", e.what());
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
