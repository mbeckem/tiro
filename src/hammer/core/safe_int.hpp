#ifndef HAMMER_CORE_SAFE_INT
#define HAMMER_CORE_SAFE_INT

#include "hammer/core/defs.hpp"
#include "hammer/core/math.hpp"

namespace hammer {

// TODO: Safe division
// TODO: Tests
template<typename T>
class SafeInt final {
public:
    SafeInt()
        : value_(0) {}
    SafeInt(T value)
        : value_(value) {}

    T value() const noexcept { return value_; }

    SafeInt& operator+=(SafeInt v) { return _add(v.value_), *this; }
    SafeInt& operator-=(SafeInt v) { return _sub(v.value_), *this; }
    SafeInt& operator*=(SafeInt v) { return _mul(v.value_), *this; }

    SafeInt operator+(SafeInt v) const { return SafeInt(*this) += v; }
    SafeInt operator-(SafeInt v) const { return SafeInt(*this) -= v; }
    SafeInt operator*(SafeInt v) const { return SafeInt(*this) *= v; }

    SafeInt& operator++() {
        _add(1);
        return *this;
    }

    SafeInt operator++(int) {
        SafeInt temp = *this;
        _add(1);
        return temp;
    }

    SafeInt& operator--() {
        _sub(1);
        return *this;
    }

    SafeInt& operator--(int) {
        SafeInt temp = *this;
        _sub(1);
        return temp;
    }

#define HAMMER_TRIVIAL_COMPARE(op)                                    \
    friend bool operator op(const SafeInt& lhs, const SafeInt& rhs) { \
        return lhs op rhs;                                            \
    }

    HAMMER_TRIVIAL_COMPARE(==)
    HAMMER_TRIVIAL_COMPARE(!=)
    HAMMER_TRIVIAL_COMPARE(<=)
    HAMMER_TRIVIAL_COMPARE(>=)
    HAMMER_TRIVIAL_COMPARE(<)
    HAMMER_TRIVIAL_COMPARE(>)

#undef HAMMER_TRIVIAL_COMPARE

private:
    void _add(T v) {
        if (HAMMER_UNLIKELY(!checked_add(value_, v, value_)))
            HAMMER_ERROR("Integer overflow in addition.");
    }

    void _sub(T v) {
        if (HAMMER_UNLIKELY(!checked_sub(value_, v, value_)))
            HAMMER_ERROR("Integer overflow in subtraction.");
    }

    void _mul(T v) {
        if (HAMMER_UNLIKELY(!checked_mul(value_, v, value_)))
            HAMMER_ERROR("Integer overflow in subtraction.");
    }

private:
    T value_;
};

} // namespace hammer

#endif // HAMMER_CORE_SAFE_INT
