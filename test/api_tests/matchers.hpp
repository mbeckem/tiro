#ifndef TIRO_TEST_API_TESTS_MATCHERS_HPP_INCLUDED
#define TIRO_TEST_API_TESTS_MATCHERS_HPP_INCLUDED

#include "tiropp/error.hpp"

#include <catch2/catch.hpp>

#include <sstream>

namespace {

class throws_code_matcher : public Catch::MatcherBase<tiro::api_error> {
public:
    throws_code_matcher(tiro::api_errc code)
        : code_(code) {}

    bool match(const tiro::api_error& e) const override { return e.code() == code_; }

    virtual std::string describe() const override {
        std::ostringstream ss;
        ss << "throws " << tiro::name(code_);
        return ss.str();
    }

private:
    tiro::api_errc code_;
};

inline throws_code_matcher throws_code(tiro::api_errc code) {
    return throws_code_matcher(code);
}

} // namespace

#endif // TIRO_TEST_API_TESTS_MATCHERS_HPP_INCLUDED
