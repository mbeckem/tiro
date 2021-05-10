#ifndef TIRO_TEST_SUPPORT_VM_MATCHERS_HPP
#define TIRO_TEST_SUPPORT_VM_MATCHERS_HPP

#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/value.hpp"

#include <exception>
#include <sstream>
#include <string_view>

namespace tiro::test_support {

class IsIntegerValue : public Catch::MatcherBase<vm::Value> {
public:
    explicit IsIntegerValue(i64 expected)
        : expected_(expected) {}

    bool match(const vm::Value& v) const override {
        return vm::Integer::try_extract(v) == expected_;
    }

    virtual std::string describe() const override {
        std::ostringstream ss;
        ss << "Value must be an integer object equal to " << expected_;
        return ss.str();
    }

private:
    i64 expected_;
};

inline IsIntegerValue is_integer_value(i64 expected) {
    return IsIntegerValue(expected);
}

} // namespace tiro::test_support

#endif // TIRO_TEST_SUPPORT_VM_MATCHERS_HPP
