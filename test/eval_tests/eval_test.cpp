#include "eval_test.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>

namespace tiro::eval_tests {

static const char* test_module_name = "test";

eval_test::eval_test(std::string_view source, int flags)
    : source_(source)
    , flags_(flags)
    , vm_()
    , result_(compile_source(source_.c_str(), flags)) {
    vm_.load_std();
    vm_.load(result_.mod);
}

eval_test::compile_result eval_test::compile_source(const char* source, int flags) {
    std::string cst, ast, ir, bytecode;
    std::string output;

    compiler_settings settings;
    settings.enable_dump_cst = flags & enable_cst;
    settings.enable_dump_ast = flags & enable_ast;
    settings.enable_dump_ir = flags & enable_ir;
    settings.enable_dump_bytecode = flags & enable_bytecode;
    settings.message_callback = [&](severity sev, uint32_t line, uint32_t column,
                                    const char* message) {
        // TODO: Must not throw.
        if (!output.empty())
            output += "\n";

        output += fmt::format("{} {}:{}: {}", to_string(sev), line, column, message);
    };

    // TODO: C++ api should use std::string_view instead of const char*
    compiler comp(settings);
    comp.add_file(test_module_name, source);

    try {
        comp.run();
    } catch (const error& e) {
        std::string combined = e.message();
        std::string details = e.details();
        if (!details.empty()) {
            combined += "\n";
            combined += details;
        }

        if (!output.empty()) {
            combined += "\n\ncompilation messages:\n";
            combined += output;
        }

        throw std::runtime_error(combined);
    }

    if (settings.enable_dump_cst)
        cst = comp.dump_cst();
    if (settings.enable_dump_ast)
        ast = comp.dump_ast();
    if (settings.enable_dump_ir)
        ir = comp.dump_ir();
    if (settings.enable_dump_bytecode)
        bytecode = comp.dump_bytecode();

    compiled_module mod = comp.take_module();
    return compile_result{
        std::move(mod),
        std::move(cst),
        std::move(ast),
        std::move(ir),
        std::move(bytecode),
    };
}

handle eval_test::as_object(std::nullptr_t) {
    return make_null(vm_);
}

handle eval_test::as_object(bool value) {
    return make_boolean(vm_, value);
}

handle eval_test::as_object(int64_t value) {
    return make_integer(vm_, value);
}

handle eval_test::as_object(double value) {
    return make_float(vm_, value);
}

handle eval_test::as_object(std::string_view value) {
    return make_string(vm_, value);
}

handle eval_test::as_object(handle value) {
    return value;
}

result eval_test::exec(const char* function_name, const std::vector<handle>& function_args) {
    auto func = get_export(vm_, test_module_name, function_name).as<function>();
    auto args = make_tuple(vm_, function_args.size());
    size_t index = 0;
    for (const auto& arg : function_args) {
        args.set(index++, arg);
    }

    tiro::handle exec_result = make_null(vm_);
    run_async(vm_, func, args, [&](vm&, const coroutine& coro) { exec_result = coro.result(); });
    while (vm_.has_ready()) {
        vm_.run_ready();
    }

    if (exec_result.kind() == value_kind::null)
        throw std::runtime_error("test function did not complete synchronously");
    return exec_result.as<result>();
}

eval_call::eval_call(eval_test& test, const char* function, std::vector<handle> args)
    : test_(test)
    , function_(function)
    , args_(std::move(args)) {}

result eval_call::run() {
    return test_.exec(function_, args_);
}

handle eval_call::returns_value() {
    auto res = run();
    REQUIRE(res.is_success());
    return res.value();
}

handle eval_call::panics() {
    auto res = run();
    REQUIRE(res.is_error());
    return res.error();
}

void eval_call::returns_bool(bool value) {
    auto res = returns_value();
    CAPTURE(to_string(res.kind()));
    REQUIRE(res.kind() == value_kind::boolean);
    REQUIRE(res.as<boolean>().value() == value);
}

void eval_call::returns_int(int64_t value) {
    auto res = returns_value();
    CAPTURE(to_string(res.kind()));
    REQUIRE(res.kind() == value_kind::integer);
    REQUIRE(res.as<integer>().value() == value);
}

void eval_call::returns_float(double value) {
    auto res = returns_value();
    CAPTURE(to_string(res.kind()));
    REQUIRE(res.kind() == value_kind::float_);
    REQUIRE(res.as<float_>().value() == value);
}

void eval_call::returns_string(std::string_view value) {
    auto res = returns_value();
    CAPTURE(to_string(res.kind()));
    REQUIRE(res.kind() == value_kind::string);
    REQUIRE(res.as<string>().view() == value);
}

} // namespace tiro::eval_tests
