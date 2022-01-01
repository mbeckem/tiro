#ifndef TIRO_TEST_EVAL_TESTS_MATCHERS_HPP_INCLUDED
#define TIRO_TEST_EVAL_TESTS_MATCHERS_HPP_INCLUDED

#include "tiropp/error.hpp"

#include "eval_test.hpp"

#include <catch2/catch.hpp>

#include <sstream>

namespace tiro::eval_tests {

class compile_error_matcher : public Catch::MatcherBase<compile_error> {
public:
    compile_error_matcher(tiro::api_errc code)
        : code_(code) {}

    bool match(const compile_error& e) const override { return e.code() == code_; }

    virtual std::string describe() const override {
        std::ostringstream ss;
        ss << "throws " << tiro::name(code_);
        return ss.str();
    }

private:
    tiro::api_errc code_;
};

// TODO: Need better infrastructure to test compiler errors.
inline compile_error_matcher throws_compile_error(tiro::api_errc code) {
    return compile_error_matcher(code);
}

class message_contains_matcher : public Catch::MatcherBase<std::exception> {
public:
    message_contains_matcher(std::string message)
        : message_(std::move(message)) {}

    bool match(const std::exception& e) const override {
        std::string_view what(e.what());
        return what.find(message_) != what.npos;
    }

    virtual std::string describe() const override {
        std::ostringstream ss;
        ss << "message contains '" << message_ << "'";
        return ss.str();
    }

private:
    std::string message_;
};

inline message_contains_matcher message_contains(std::string message) {
    return message_contains_matcher(std::move(message));
}

} // namespace tiro::eval_tests

#endif // TIRO_TEST_EVAL_TESTS_MATCHERS_HPP_INCLUDED
