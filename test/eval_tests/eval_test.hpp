#ifndef TIRO_TEST_EVAL_TESTS_EVAL_TEST_HPP_INCLUDED
#define TIRO_TEST_EVAL_TESTS_EVAL_TEST_HPP_INCLUDED

#include "tiropp/api.hpp"

#include <string>
#include <type_traits>
#include <vector>

namespace tiro::eval_tests {

class eval_test;
class eval_call;

class eval_test final {
public:
    // OR these together in the `flags` constructor paramater to enable
    // the corresponding dump_* methods.
    // They are disabled by default for better performance.
    enum eval_flags {
        enable_cst = 1 << 0,
        enable_ast = 1 << 1,
        enable_ir = 1 << 2,
        enable_bytecode = 1 << 3,
    };

    explicit eval_test(std::string_view source, int flags = 0);

    eval_test(const eval_test&) = delete;
    eval_test& operator=(const eval_test&) = delete;

    const std::string& source() { return source_; }
    int flags() { return flags_; }

    const std::string& dump_cst() { return result_.cst; }
    const std::string& dump_ast() { return result_.ast; }
    const std::string& dump_ir() { return result_.ir; }
    const std::string& dump_bytecode() { return result_.bytecode; }

    template<typename... Args>
    [[nodiscard]] eval_call call(const char* function, Args&&... args);

private:
    struct compile_result {
        compiled_module mod;
        std::string cst;
        std::string ast;
        std::string ir;
        std::string bytecode;
    };

    static compile_result compile_source(const char* content, int flags);

private:
    friend eval_call;

    handle as_object(std::nullptr_t value);
    handle as_object(bool value);

    handle as_object(int64_t value);

    template<typename T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
    handle as_object(T value) {
        return as_object(static_cast<int64_t>(value));
    }

    handle as_object(double value);
    handle as_object(std::string_view value);
    handle as_object(handle value);

    result exec(const char* function, const std::vector<handle>& args);

private:
    std::string source_;
    int flags_ = 0;
    vm vm_;
    compile_result result_;
};

class [[nodiscard]] eval_call final {
public:
    eval_call(const eval_call&) = delete;
    eval_call& operator=(const eval_call&) = delete;

    result run();
    handle returns_value();
    handle panics();

    void returns_bool(bool value);
    void returns_int(int64_t value);
    void returns_float(double value);
    void returns_string(std::string_view value);

private:
    friend eval_test;

    explicit eval_call(eval_test & test, const char* function, std::vector<handle> args);

private:
    eval_test& test_;
    const char* function_;
    std::vector<handle> args_;
};

template<typename... Args>
inline eval_call eval_test::call(const char* function, Args&&... args) {
    std::vector<handle> function_args{as_object(std::forward<Args>(args))...};
    return eval_call(*this, function, std::move(function_args));
}

} // namespace tiro::eval_tests

#endif // TIRO_TEST_EVAL_TESTS_EVAL_TEST_HPP_INCLUDED