#include "cxxopts.hpp"
#include "fmt/format.h"
#include "tiropp/api.hpp"

#include <cstdio>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>

namespace {

struct OptionsError {
    std::string message;
};

struct ShowHelp {
    std::string content;
};

struct Options {
    std::string input;
    std::optional<std::string> call;
    bool dump_cst = false;
    bool dump_ast = false;
    bool dump_ir = false;
    bool dump_bytecode = false;
};

using OptionsResult = std::variant<Options, ShowHelp, OptionsError>;

OptionsResult parse_options(int argc, char** argv);
tiro::compiled_module compile(std::string_view content, const Options& options);
int run(const tiro::compiled_module& module, std::string_view function_name);
std::string read_file_contents(const char* path);

static const std::string_view test_module_name = "test";

} // namespace

int main(int argc, char* argv[]) {
    try {
        OptionsResult options_result = parse_options(argc, argv);
        if (auto err = std::get_if<OptionsError>(&options_result)) {
            fmt::print(stderr, "{}\n", err->message);
            return 1;
        }
        if (auto help = std::get_if<ShowHelp>(&options_result)) {
            fmt::print(stdout, "{}\n", help->content);
            return 0;
        }

        Options options = std::get<Options>(options_result);
        std::string content;
        try {
            content = read_file_contents(options.input.c_str());
        } catch (const std::exception& e) {
            fmt::print(stderr, "Failed to read '{}': {}\n", options.input, e.what());
            return 1;
        }

        std::optional<tiro::compiled_module> compiled;
        try {
            compiled = compile(content, options);
        } catch (const std::exception& e) {
            fmt::print(stderr, "Compilation failed: {}\n", e.what());
            return 1;
        }

        if (options.call)
            return run(*compiled, *options.call);
    } catch (const std::exception& e) {
        fmt::print(stderr, "Fatal error: {}\n", e.what());
        return 1;
    }
    return 0;
}

namespace {

OptionsResult parse_options(int argc, char** argv) {
    cxxopts::Options options(
        argv[0], "simple runner for executing tiro scripts and inspecting internals");
    options.positional_help("<input>");

    /* clang-format off */
    options.add_options()
        ("call", "call the exported function with the given name", cxxopts::value<std::string>(), "<name>")
        ("dump-cst", "print the compiler's cst", cxxopts::value<bool>())
        ("dump-ast", "print the compiler's ast", cxxopts::value<bool>())
        ("dump-ir", "print the compiler's intermediate representation", cxxopts::value<bool>())
        ("dump-bytecode", "print the disassembled final bytecode", cxxopts::value<bool>())
        ("dump", "dump all intermediate datastructures", cxxopts::value<bool>())
        ("input", "input file", cxxopts::value<std::string>(), "<file>")
        ("h,help", "show this message", cxxopts::value<bool>());
    /* clang-format on */
    options.parse_positional("input");

    // Work around missing default constructor
    std::optional<cxxopts::ParseResult> opt_result;
    try {
        opt_result.emplace(options.parse(argc, argv));
    } catch (const cxxopts::OptionException& e) {
        return OptionsError{fmt::format("Error: {}", e.what())};
    }

    auto& result = *opt_result;
    if (result.count("help")) {
        return ShowHelp{options.help()};
    }

    Options parsed_options;
    if (auto input = result["input"]; input.count()) {
        parsed_options.input = input.as<std::string>();
    } else {
        return OptionsError{"Error: input file is required"};
    }

    if (result["dump"].as<bool>()) {
        parsed_options.dump_cst = parsed_options.dump_ast = parsed_options.dump_ir =
            parsed_options.dump_bytecode = true;
    }
    if (result["dump-cst"].as<bool>())
        parsed_options.dump_cst = true;
    if (result["dump-ast"].as<bool>())
        parsed_options.dump_ast = true;
    if (result["dump-ir"].as<bool>())
        parsed_options.dump_ir = true;
    if (result["dump-bytecode"].as<bool>())
        parsed_options.dump_bytecode = true;
    if (auto call = result["call"]; call.count())
        parsed_options.call = call.as<std::string>();
    return parsed_options;
}

tiro::compiled_module compile(std::string_view content, const Options& options) {
    tiro::compiler_settings settings;
    settings.enable_dump_cst = options.dump_cst;
    settings.enable_dump_ast = options.dump_ast;
    settings.enable_dump_ir = options.dump_ir;
    settings.enable_dump_bytecode = options.dump_bytecode;
    settings.message_callback = [](tiro::severity sev, uint32_t line, uint32_t column,
                                    std::string_view message) {
        fmt::print("{} {}:{}:{}\n", to_string(sev), line, column, message);
    };

    std::exception_ptr error;
    std::optional<std::string> cst, ast, ir, bytecode;
    auto try_extract = [](auto fn) -> std::optional<decltype(fn())> {
        try {
            return fn();
        } catch (...) {
            return {};
        }
    };

    tiro::compiler compiler(settings);
    compiler.add_file(test_module_name, content);
    try {
        compiler.run();
    } catch (...) {
        error = std::current_exception();
    }

    // Print as much as possible, regardless of errors
    cst = try_extract([&] { return compiler.dump_cst(); });
    ast = try_extract([&] { return compiler.dump_ast(); });
    ir = try_extract([&] { return compiler.dump_ir(); });
    bytecode = try_extract([&] { return compiler.dump_bytecode(); });
    for (const auto* opt : {&cst, &ast, &ir, &bytecode}) {
        if (opt->has_value())
            fmt::print("{}\n\n", **opt);
    }

    if (error)
        std::rethrow_exception(error);
    return compiler.take_module();
}

int run(const tiro::compiled_module& module, std::string_view function_name) {
    tiro::vm vm;
    vm.load_std();
    vm.load(module);

    std::optional<tiro::handle> target;
    try {
        target = tiro::get_export(vm, test_module_name, function_name);
    } catch (const tiro::error& e) {
        fmt::print(stderr, "Failed to retrieve function '{}': {}\n", function_name, e.message());
        return 1;
    }

    if (target->kind() != tiro::value_kind::function) {
        fmt::print(stderr, "Exported member is not a function\n");
        return 1;
    }

    std::optional<int> exit;
    tiro::run_async(vm, target->as<tiro::function>(), [&](tiro::vm&, const tiro::coroutine& coro) {
        tiro::result result = coro.result();
        if (result.is_success()) {
            fmt::print("Function returned {}\n", result.value().to_string().view());
            exit = 0;
        } else {
            fmt::print("Function panicked: {}\n", result.error().to_string().view());
            exit = 1;
        }
    });

    while (vm.has_ready()) {
        vm.run_ready();
    }

    if (!exit)
        throw std::logic_error("function did not return after main loop completed");
    return *exit;
}

std::string read_file_contents(const char* path) {
    struct file_handle {
        FILE* fd = nullptr;

        ~file_handle() {
            if (fd)
                std::fclose(fd);
        }
    } file;

    file.fd = std::fopen(path, "rb");
    if (!file.fd) {
        throw std::system_error(errno, std::system_category());
    };

    // Use the file size as a hint for the required memory (the file size might change while we're reading it).
    std::string result;
    {
        auto size = std::filesystem::file_size(path);
        if (size > result.max_size()) {
            throw std::runtime_error("file is too large");
        }
        result.reserve(size);
    }

    // Read file chunk by chunk.
    unsigned char buffer[4096];
    while (1) {
        size_t read = std::fread(buffer, 1, sizeof(buffer), file.fd);
        if (ferror(file.fd)) {
            throw std::system_error(errno, std::system_category());
        }

        result.append(buffer, buffer + read);
        if (feof(file.fd)) {
            break;
        }
    }

    return result;
}

} // namespace
