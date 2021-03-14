#ifndef TIRO_TEST_SUPPORT_MATCHERS_HPP
#define TIRO_TEST_SUPPORT_MATCHERS_HPP

#include <catch2/catch.hpp>

#include <exception>
#include <sstream>
#include <string_view>

namespace tiro::test_support {

class ExceptionContainsString : public Catch::MatcherBase<std::exception> {
public:
    explicit ExceptionContainsString(std::string str)
        : str_(std::move(str)) {}

    bool match(const std::exception& e) const override {
        std::string_view what = e.what();
        return what.find(str_) != std::string_view::npos;
    }

    virtual std::string describe() const override {
        std::ostringstream ss;
        ss << "Exception must contain string '" << str_ << "'";
        return ss.str();
    }

private:
    std::string str_;
};

inline ExceptionContainsString exception_contains_string(std::string str) {
    return ExceptionContainsString(std::move(str));
}

} // namespace tiro::test_support

#endif // TIRO_TEST_SUPPORT_MATCHERS_HPP
