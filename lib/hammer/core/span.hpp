#ifndef HAMMER_CORE_SPAN_HPP
#define HAMMER_CORE_SPAN_HPP

#include "hammer/core/defs.hpp"

#include <array>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <vector>

/**
 * A pointer + length pair (with debug mode boundschecking) for unowned arrays.
 * Similar to std::span, which is not yet available.
 */
template<typename T>
class Span {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;

    using iterator = pointer;
    using const_iterator = pointer;

    /// Constructs an empty span.
    constexpr Span() noexcept = default;

    /// Constructs a one-element span.
    constexpr Span(T& value) noexcept
        : data_(std::addressof(value))
        , size_(1) {}

    /// Constructs a span from a pointer + length pair.
    constexpr Span(pointer ptr, size_type count) noexcept
        : data_(ptr)
        , size_(count) {}

    /// Constructs a span from a pointer pair [first, last).
    constexpr Span(pointer first, pointer last) noexcept
        : data_(first)
        , size_(static_cast<size_t>(last - first)) {}

    /// Construct a span from an array of static size.
    template<std::size_t N>
    constexpr Span(value_type (&array)[N]) noexcept
        : data_(array)
        , size_(N) {}

    /// Constructs a span from a vector.
    template<typename U, typename Alloc,
        std::enable_if_t<std::is_same_v<T, const U>>* = nullptr>
    Span(const std::vector<U, Alloc>& vec) noexcept
        : data_(vec.data())
        , size_(vec.size()) {}

    /// Constructs a span from a vector.
    template<typename U, typename Alloc,
        std::enable_if_t<std::is_same_v<const T, const U>>* = nullptr>
    Span(std::vector<U, Alloc>& vec) noexcept
        : data_(vec.data())
        , size_(vec.size()) {}

    /// Constructs a span from an std::array.
    template<typename U, size_t N,
        std::enable_if_t<std::is_same_v<T, const U>>* = nullptr>
    Span(const std::array<U, N>& arr) noexcept
        : data_(arr.data())
        , size_(N) {}

    /// Constructs a span from an std::array.
    template<typename U, size_t N,
        std::enable_if_t<std::is_same_v<const T, const U>>* = nullptr>
    Span(std::array<U, N>& arr) noexcept
        : data_(arr.data())
        , size_(N) {}

    /// Construct a span from an initializer list.
    template<typename U,
        std::enable_if_t<std::is_same_v<T, const U>>* = nullptr>
    Span(std::initializer_list<U> list) noexcept
        : data_(list.begin())
        , size_(list.size()) {}

    /// Conversion non-const -> const.
    template<typename U,
        std::enable_if_t<std::is_same_v<T, const U>>* = nullptr>
    constexpr Span(const Span<U>& other) noexcept
        : data_(other.data_)
        , size_(other.size_) {}

    constexpr Span(const Span& other) noexcept = default;
    constexpr Span& operator=(const Span& other) = default;

    const_iterator begin() const { return data_; }
    const_iterator end() const { return data_ + size_; }

    /// Returns a reference to the first element.
    T& front() const noexcept {
        HAMMER_ASSERT(size_ > 0, "Span::front(): span is empty.");
        return data_[0];
    }

    /// Returns a reference to the last element.
    T& back() const noexcept {
        HAMMER_ASSERT(size_ > 0, "Span::back(): span is empty.");
        return data_[size_ - 1];
    }

    /// Returns a reference to the element at `index`.
    T& operator[](size_t index) const noexcept {
        HAMMER_ASSERT(
            index < size_, "Span::operator[](): index is out of bounds.");
        return data_[index];
    }

    /// Returns a pointer to the span's backing storage. `size()` elements starting
    /// from that pointer may be dereferenced.
    T* data() const noexcept { return data_; }

    /// Returns the number of elements in this span.
    size_t size() const noexcept { return size_; }

    /// Returns the total size of the elements in this span, in bytes.
    size_t size_bytes() const noexcept { return size_ * sizeof(T); }

    /// Returns true if the span is empty.
    bool empty() const noexcept { return size_ == 0; }

    /// Returns subspan over the first `count` values.
    Span first(size_t count) const noexcept {
        HAMMER_ASSERT(count <= size_, "Span::first(): count is too large.");
        return {data_, count};
    }

    /// Returns a subspan over the last `count` values.
    Span last(size_t count) const noexcept {
        HAMMER_ASSERT(count <= size_, "Span::last(): count is too large.");
        return {data_ + (size_ - count), count};
    }

    /// Returns a sub starting from `offset` with `count` values.
    Span subspan(size_t offset, size_t count) const noexcept {
        HAMMER_ASSERT(
            offset <= size_, "Span::subspan(): offset is out of bounds.");
        HAMMER_ASSERT(
            count <= size_ - offset, "Span::subspan(): count is too large.");
        return {data_ + offset, count};
    }

    /// Returns a subspan without the first `count` values.
    Span drop_front(size_t count) const noexcept {
        HAMMER_ASSERT(
            count <= size_, "Span::drop_front(): count is too large.");
        return {data_ + count, size_ - count};
    }

    /// Returns a subspan without the last `count` values.
    Span drop_back(size_t count) const noexcept {
        HAMMER_ASSERT(count <= size_, "Span::drop_back(): count is too large.");
        return {data_ + (size_ - count), size_ - count};
    }

private:
    T* data_ = nullptr;
    size_t size_ = 0;
};

#endif // HAMMER_CORE_SPAN_HPP
