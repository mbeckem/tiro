#ifndef TIRO_COMPILER_SEMANTICS_RESET_VALUE_HPP
#define TIRO_COMPILER_SEMANTICS_RESET_VALUE_HPP

#include <utility>

namespace tiro::compiler {

/// Replaces a value with an old value in its destructor.
/// Useful for recursive algoriths in the tree visitors.
template<typename T>
struct [[nodiscard]] ResetValue {
    T& location_;
    T old_;

    ResetValue(T & location, T old)
        : location_(location)
        , old_(std::move(old)) {}

    ~ResetValue() { location_ = std::move(old_); }

    ResetValue(const ResetValue&) = delete;
    ResetValue& operator=(const ResetValue&) = delete;
};

} // namespace tiro::compiler

#endif // TIRO_COMPILER_SEMANTICS_RESET_VALUE_HPP