#include "eval_test.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>

namespace tiro::eval_tests {

static const std::string_view test_module_name = "test";

static vm_settings create_vm_settings(int flags) {
    vm_settings settings;
    settings.enable_panic_stack_traces = flags & eval_test::enable_panic_stack_traces;
    return settings;
}

eval_test::eval_test(eval_spec spec, int flags)
    : spec_(std::move(spec))
    , flags_(flags)
    , vm_(create_vm_settings(flags))
    , result_(compile_sources(spec_.sources, flags)) {
    vm_.load_std();
    vm_.load(result_.mod);
}

std::string_view eval_test::module_name() {
    return test_module_name;
}

handle eval_test::get_export(std::string_view name) {
    return tiro::get_export(vm_, module_name(), name);
}

eval_test::compile_result
eval_test::compile_sources(const std::vector<std::string>& sources, int flags) {
    std::string cst, ast, ir, bytecode;
    std::string output;

    compiler comp(test_module_name);
    if (flags & enable_cst)
        comp.request_attachment(attachment::cst);
    if (flags & enable_ast)
        comp.request_attachment(attachment::ast);
    if (flags & enable_ir)
        comp.request_attachment(attachment::ir);
    if (flags & enable_bytecode)
        comp.request_attachment(attachment::bytecode);
    comp.set_message_callback([&](const compiler_message& message) {
        // TODO: Must not throw.
        if (!output.empty())
            output += "\n";

        output += fmt::format("{} {}:{}:{}: {}", to_string(message.severity), message.file,
            message.line, message.column, message.text);
    });

    for (size_t i = 0, n = sources.size(); i < n; ++i) {
        comp.add_file(fmt::format("input-{}", i), sources[i]);
    }

    try {
        comp.run();
    } catch (const api_error& e) {
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

        throw compile_error(e.code(), std::move(combined));
    }

    if (flags & enable_cst)
        cst = comp.get_attachment(attachment::cst);
    if (flags & enable_ast)
        ast = comp.get_attachment(attachment::ast);
    if (flags & enable_ir)
        ir = comp.get_attachment(attachment::ir);
    if (flags & enable_bytecode)
        bytecode = comp.get_attachment(attachment::bytecode);

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

handle eval_test::as_object(const char* value) {
    return make_string(vm_, value);
}

handle eval_test::as_object(std::string_view value) {
    return make_string(vm_, value);
}

handle eval_test::as_object(handle value) {
    return value;
}

result eval_test::exec(std::string_view function_name, const std::vector<handle>& function_args) {
    auto func = get_export(function_name).as<function>();
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

eval_call::eval_call(eval_test& test, std::string_view function, std::vector<handle> args)
    : test_(test)
    , function_(function)
    , args_(std::move(args)) {}

result eval_call::run() {
    return test_.exec(function_, args_);
}

handle eval_call::returns_value() {
    auto res = run();
    if (!res.is_success()) {
        auto ex = res.error().as<tiro::exception>();
        FAIL("exception: " << ex.message().value());
    }
    return res.value();
}

handle eval_call::panics() {
    auto res = run();
    REQUIRE(res.is_error());
    return res.error();
}

void eval_call::returns_null() {
    auto res = returns_value();
    CAPTURE(to_string(res.kind()));
    REQUIRE(res.kind() == value_kind::null);
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
