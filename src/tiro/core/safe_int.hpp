#ifndef TIRO_CORE_SAFE_INT
#define TIRO_CORE_SAFE_INT

#include "tiro/core/defs.hpp"
#include "tiro/core/math.hpp"

namespace tiro {

template<typename T>
class SafeInt final {
public:
    using Limits = std::numeric_limits<T>;

    SafeInt()
        : value_(0) {}
    SafeInt(T value)
        : value_(value) {}

    T value() const noexcept { return value_; }

    SafeInt& operator+=(SafeInt v) { return _add_throws(v.value_), *this; }
    SafeInt& operator-=(SafeInt v) { return _sub_throws(v.value_), *this; }
    SafeInt& operator*=(SafeInt v) { return _mul_throws(v.value_), *this; }
    SafeInt& operator/=(SafeInt v) { return _div_throws(v.value_), *this; }
    SafeInt& operator%=(SafeInt v) { return _mod_throws(v.value_), *this; }

    SafeInt operator+(SafeInt v) const { return SafeInt(*this) += v; }
    SafeInt operator-(SafeInt v) const { return SafeInt(*this) -= v; }
    SafeInt operator*(SafeInt v) const { return SafeInt(*this) *= v; }
    SafeInt operator/(SafeInt v) const { return SafeInt(*this) /= v; }
    SafeInt operator%(SafeInt v) const { return SafeInt(*this) %= v; }

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
        if (TIRO_UNLIKELY(!checked_add(value_, v, value_)))
            return false;
        return true;
    }

    bool try_sub(T v) noexcept {
        if (TIRO_UNLIKELY(!checked_sub(value_, v, value_)))
            return false;
        return true;
    }

    bool try_mul(T v) noexcept {
        if (TIRO_UNLIKELY(!checked_mul(value_, v, value_)))
            return false;
        return true;
    }

    bool try_div(T v) noexcept {
        if (TIRO_UNLIKELY(!checked_div(value_, v, value_)))
            return false;
        return true;
    }

    bool try_mod(T v) noexcept {
        if (TIRO_UNLIKELY(!checked_mod(value_, v, value_)))
            return false;
        return true;
    }

#define TIRO_TRIVIAL_COMPARE(op)                                      \
    friend bool operator op(const SafeInt& lhs, const SafeInt& rhs) { \
        return lhs.value_ op rhs.value_;                              \
    }

    TIRO_TRIVIAL_COMPARE(==)
    TIRO_TRIVIAL_COMPARE(!=)
    TIRO_TRIVIAL_COMPARE(<=)
    TIRO_TRIVIAL_COMPARE(>=)
    TIRO_TRIVIAL_COMPARE(<)
    TIRO_TRIVIAL_COMPARE(>)

#undef TIRO_TRIVIAL_COMPARE

private:
    void _add_throws(T v) {
        if (!try_add(v))
            TIRO_ERROR("Integer overflow in addition.");
    }

    void _sub_throws(T v) {
        if (!try_sub(v))
            TIRO_ERROR("Integer overflow in subtraction.");
    }

    void _mul_throws(T v) {
        if (!try_mul(v))
            TIRO_ERROR("Integer overflow in subtraction.");
    }

    void _div_throws(T v) {
        if (!try_div(v))
            TIRO_ERROR("Arithmetic error in division.");
    }

    void _mod_throws(T v) {
        if (!try_mod(v))
            TIRO_ERROR("Arithmetic error in remainder.");
    }

private:
    T value_;
};

} // namespace tiro

#endif // TIRO_CORE_SAFE_INT
