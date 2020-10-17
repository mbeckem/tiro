#ifndef TIRO_COMMON_ADT_SPAN_HPP
#define TIRO_COMMON_ADT_SPAN_HPP

#include "common/assert.hpp"
#include "common/defs.hpp"
#include "common/type_traits.hpp"

namespace tiro {

template<typename T>
class Span;

namespace detail {

template<typename T>
struct IsSpan : std::false_type {};

template<typename T>
struct IsSpan<Span<T>> : std::true_type {};

template<typename T, typename Enabled = void>
struct HasData : std::false_type {};

template<typename T>
struct HasData<T, std::void_t<decltype(std::declval<T>().data())>> : std::true_type {};

template<typename T, typename Enabled = void>
struct HasSize : std::false_type {};

template<typename T>
struct HasSize<T, std::void_t<decltype(std::declval<T>().size())>> : std::true_type {};

template<typename Container>
struct ContainerValueType {
    using type = typename Container::value_type;
};

template<typename T, size_t N>
struct ContainerValueType<T[N]> {
    using type = T;
};

template<typename T>
using container_value_type = typename ContainerValueType<remove_cvref_t<T>>::type;

template<typename T>
using disable_if_span = std::enable_if_t<!IsSpan<remove_cvref_t<T>>::value>;

template<typename T>
inline constexpr bool span_convertible = HasData<T>::value&& HasSize<T>::value;

} // namespace detail

/// A pointer + length pair (with debug mode bounds checking) for unowned arrays.
/// Similar to std::span, which is not yet available.
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

    /// Constructs a span from a pointer + length pair.
    constexpr Span(pointer ptr, size_type count) noexcept
        : data_(ptr)
        , size_(count) {}

    /// Constructs a span from a pointer pair [first, last).
    constexpr Span(pointer first, pointer last) noexcept
        : data_(first)
        , size_(static_cast<size_t>(last - first)) {}

    /// Construct a span from a container that supports data() and size().
    template<typename Container, detail::disable_if_span<Container>* = nullptr>
    constexpr Span(Container&& cont)
        : data_(std::data(cont))
        , size_(std::size(cont)) {
        static_assert(
            std::is_same_v<const std::remove_pointer_t<decltype(std::data(cont))>, const T>,
            "Container value type must match the span's type.");
    }

    /// Conversion non-const -> const.
    template<typename U, std::enable_if_t<std::is_same_v<T, const U>>* = nullptr>
    constexpr Span(const Span<U>& other) noexcept
        : data_(other.data_)
        , size_(other.size_) {}

    constexpr Span(const Span& other) noexcept = default;
    constexpr Span& operator=(const Span& other) = default;

    constexpr const_iterator begin() const { return data_; }
    constexpr const_iterator end() const { return data_ + size_; }

    /// Returns a reference to the first element.
    constexpr T& front() const noexcept {
        TIRO_DEBUG_ASSERT(size_ > 0, "Span::front(): span is empty.");
        return data_[0];
    }

    /// Returns a reference to the last element.
    constexpr T& back() const noexcept {
        TIRO_DEBUG_ASSERT(size_ > 0, "Span::back(): span is empty.");
        return data_[size_ - 1];
    }

    /// Returns a reference to the element at `index`.
    constexpr T& operator[](size_t index) const noexcept {
        TIRO_DEBUG_ASSERT(index < size_, "Span::operator[](): index is out of bounds.");
        return data_[index];
    }

    /// Returns a pointer to the span's backing storage. `size()` elements starting
    /// from that pointer may be dereferenced.
    constexpr T* data() const noexcept { return data_; }

    /// Returns the number of elements in this span.
    constexpr size_t size() const noexcept { return size_; }

    /// Returns the total size of the elements in this span, in bytes.
    constexpr size_t size_bytes() const noexcept { return size_ * sizeof(T); }

    /// Returns true if the span is empty.
    constexpr bool empty() const noexcept { return size_ == 0; }

    /// Returns subspan over the first `count` values.
    constexpr Span first(size_t count) const noexcept {
        TIRO_DEBUG_ASSERT(count <= size_, "Span::first(): count is too large.");
        return {data_, count};
    }

    /// Returns a subspan over the last `count` values.
    constexpr Span last(size_t count) const noexcept {
        TIRO_DEBUG_ASSERT(count <= size_, "Span::last(): count is too large.");
        return {data_ + (size_ - count), count};
    }

    /// Returns a sub starting from `offset` with `count` values.
    constexpr Span subspan(size_t offset, size_t count) const noexcept {
        TIRO_DEBUG_ASSERT(offset <= size_, "Span::subspan(): offset is out of bounds.");
        TIRO_DEBUG_ASSERT(count <= size_ - offset, "Span::subspan(): count is too large.");
        return {data_ + offset, count};
    }

    /// Returns a subspan without the first `count` values.
    constexpr Span drop_front(size_t count) const noexcept {
        TIRO_DEBUG_ASSERT(count <= size_, "Span::drop_front(): count is too large.");
        return {data_ + count, size_ - count};
    }

    /// Returns a subspan without the last `count` values.
    constexpr Span drop_back(size_t count) const noexcept {
        TIRO_DEBUG_ASSERT(count <= size_, "Span::drop_back(): count is too large.");
        return {data_ + (size_ - count), size_ - count};
    }

    /// Returns a span over the raw bytes of the original span.
    constexpr Span<const byte> as_bytes() const noexcept {
        return {reinterpret_cast<const byte*>(data_), size_ * sizeof(T)};
    }

private:
    template<typename U>
    friend class Span;

private:
    T* data_ = nullptr;
    size_t size_ = 0;
};

template<typename Container, std::enable_if_t<detail::span_convertible<Container>>* = nullptr>
Span(Container&)->Span<typename detail::container_value_type<Container>>;

template<typename Container, std::enable_if_t<detail::span_convertible<Container>>* = nullptr>
Span(const Container&)->Span<const typename detail::container_value_type<Container>>;

/// Returns a span over the raw storage bytes of the given value.
template<typename T>
Span<const byte> raw_span(const T& value) {
    return {reinterpret_cast<const byte*>(std::addressof(value)), sizeof(T)};
}

} // namespace tiro

#endif // TIRO_COMMON_ADT_SPAN_HPP
