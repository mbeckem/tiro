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

    SafeInt& operator+=(SafeInt v) { return _add_throws(v.value_), *this; }
    SafeInt& operator-=(SafeInt v) { return _sub_throws(v.value_), *this; }
    SafeInt& operator*=(SafeInt v) { return _mul_throws(v.value_), *this; }

    SafeInt operator+(SafeInt v) const { return SafeInt(*this) += v; }
    SafeInt operator-(SafeInt v) const { return SafeInt(*this) -= v; }
    SafeInt operator*(SafeInt v) const { return SafeInt(*this) *= v; }

    SafeInt& operator++() {
        _add_throws(1);
        return *this;
    }

    SafeInt operator++(int) {
        SafeInt temp = *this;
        _add_throws(1);
        return temp;
    }

    SafeInt& operator--() {
        _sub_throws(1);
        return *this;
    }

    SafeInt& operator--(int) {
        SafeInt temp = *this;
        _sub_throws(1);
        return temp;
    }

    bool try_add(T v) noexcept {
        if (HAMMER_UNLIKELY(!checked_add(value_, v, value_)))
            return false;
        return true;
    }

    bool try_sub(T v) noexcept {
        if (HAMMER_UNLIKELY(!checked_sub(value_, v, value_)))
            return false;
        return true;
    }

    bool try_mul(T v) noexcept {
        if (HAMMER_UNLIKELY(!checked_mul(value_, v, value_)))
            return false;
        return true;
    }

#define HAMMER_TRIVIAL_COMPARE(op)                                    \
    friend bool operator op(const SafeInt& lhs, const SafeInt& rhs) { \
        return lhs.value_ op rhs.value_;                              \
    }

    HAMMER_TRIVIAL_COMPARE(==)
    HAMMER_TRIVIAL_COMPARE(!=)
    HAMMER_TRIVIAL_COMPARE(<=)
    HAMMER_TRIVIAL_COMPARE(>=)
    HAMMER_TRIVIAL_COMPARE(<)
    HAMMER_TRIVIAL_COMPARE(>)

#undef HAMMER_TRIVIAL_COMPARE

private:
    void _add_throws(T v) {
        if (HAMMER_UNLIKELY(!checked_add(value_, v, value_)))
            HAMMER_ERROR("Integer overflow in addition.");
    }

    void _sub_throws(T v) {
        if (HAMMER_UNLIKELY(!checked_sub(value_, v, value_)))
            HAMMER_ERROR("Integer overflow in subtraction.");
    }

    void _mul_throws(T v) {
        if (HAMMER_UNLIKELY(!checked_mul(value_, v, value_)))
            HAMMER_ERROR("Integer overflow in subtraction.");
    }

private:
    T value_;
};

} // namespace hammer

#endif // HAMMER_CORE_SAFE_INT
