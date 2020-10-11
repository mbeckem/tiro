#ifndef TIRO_COMMON_ADT_VEC_PTR_HPP
#define TIRO_COMMON_ADT_VEC_PTR_HPP

#include "common/defs.hpp"

#include <functional>
#include <utility>

namespace tiro {

/// Represents an element within a vector, addressed by its index.
/// This pointer keeps a reference to the vector and an index to the element,
/// so the vector can be modified, but may not be destroyed while the pointer is in use.
template<typename T, typename Vec>
class VecPtr final {
public:
    using VectorType = std::conditional_t<std::is_const_v<T>, const Vec, Vec>;

    /// Constructs an invalid pointer.
    VecPtr()
        : VecPtr(nullptr) {}

    /// Constructs an invalid pointer.
    VecPtr(std::nullptr_t)
        : vec_(nullptr)
        , index_(0) {}

    /// Constructs a valid pointer into the vector. The `index` must be within the vector's bounds.
    VecPtr(VectorType& vec, size_t index)
        : vec_(std::addressof(vec))
        , index_(index) {
        TIRO_DEBUG_ASSERT(index < vec.size(), "Vector index is out of bounds.");
    }

    /// A pointer is valid if it points to a valid index within the vector.
    bool valid() const noexcept { return vec_ && index_ < vec_->size(); }
    explicit operator bool() const noexcept { return valid(); }

    /// Returns the address of the vector or nullptr if the pointer was constucted without a vector.
    VectorType* vec() const { return vec_; }

    /// Returns the index of the element within the vector.
    size_t index() const { return index_; }

    /// Returns the raw address of the object or nullptr if the pointer is not valid.
    T* get() const { return valid() ? data_impl() : nullptr; }

    T* operator->() const { return data_impl(); }
    T& operator*() const { return *data_impl(); }

    void reset() { *this = VecPtr(); }
    void reset(VectorType& vec, size_t index) { *this = VecPtr(vec, index); }

private:
    T* data_impl() const {
        TIRO_DEBUG_ASSERT(valid(), "Invalid VecPtr.");
        return vec_->data() + index_;
    }

private:
    VectorType* vec_;
    size_t index_;
};

template<typename Vec>
VecPtr(Vec& vec, size_t index) -> VecPtr<typename Vec::value_type, Vec>;

template<typename Vec>
VecPtr(const Vec& vec, size_t index) -> VecPtr<const typename Vec::value_type, Vec>;

#define TIRO_COMPARE(op, cmp)                                                \
    template<typename L, typename R, typename Vec>                           \
    bool operator op(const VecPtr<L, Vec>& lhs, const VecPtr<R, Vec>& rhs) { \
        return cmp<>()(lhs.get(), rhs.get());                                \
    }                                                                        \
                                                                             \
    template<typename L, typename Vec>                                       \
    bool operator op(const VecPtr<L, Vec>& lhs, std::nullptr_t) {            \
        return cmp<>()(lhs.get(), nullptr);                                  \
    }                                                                        \
                                                                             \
    template<typename R, typename Vec>                                       \
    bool operator op(std::nullptr_t, const VecPtr<R, Vec>& rhs) {            \
        return cmp<>()(nullptr, rhs.get());                                  \
    }

TIRO_COMPARE(==, std::equal_to)
TIRO_COMPARE(!=, std::not_equal_to)
TIRO_COMPARE(>=, std::greater_equal)
TIRO_COMPARE(<=, std::less_equal)
TIRO_COMPARE(>, std::greater)
TIRO_COMPARE(<, std::less)

#undef TIRO_COMPARE

} // namespace tiro

#endif // TIRO_COMMON_ADT_VEC_PTR_HPP
