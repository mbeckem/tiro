#ifndef TIRO_COMMON_ITER_TOOLS_HPP
#define TIRO_COMMON_ITER_TOOLS_HPP

#include "common/defs.hpp"
#include "common/type_traits.hpp"

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace tiro {

namespace detail {

template<typename T, typename = void>
struct IsReversible : std::false_type {};

template<typename T>
struct IsReversible<T,
    std::void_t<decltype(std::declval<T>().rbegin()), decltype(std::declval<T>().rend())>>
    : std::true_type {};

} // namespace detail

// TODO: Use a lightweight (!) range library or the standard one if it ever becomes available.

/// A pair of iterators that can be iterated using a range based for loop.
template<typename Iter>
class IterRange {
public:
    using iterator = Iter;
    using const_iterator = Iter;
    using value_type = typename std::iterator_traits<Iter>::value_type;

    IterRange(Iter b, Iter e)
        : begin_(std::move(b))
        , end_(std::move(e)) {}

    const Iter& begin() const { return begin_; }
    const Iter& end() const { return end_; }

private:
    Iter begin_;
    Iter end_;
};

/// Returns a range view over the given range. The view stores iterators into the original range,
/// but does not copy the range.
template<typename RangeLike>
auto range_view(const RangeLike& range) {
    return IterRange(range.begin(), range.end());
}

template<typename RangeLike>
auto reverse_view(const RangeLike& range) {
    if constexpr (detail::IsReversible<RangeLike>::value) {
        return IterRange(range.rbegin(), range.rend());
    } else {
        return IterRange(
            std::make_reverse_iterator(range.end()), std::make_reverse_iterator(range.begin()));
    }
}

/// Iterates over all integers in the range [min, max).
template<typename Integer>
class CountingRange {
public:
    using value_type = Integer;

    class iterator;
    using const_iterator = iterator;

    CountingRange(const Integer& min, const Integer& max)
        : min_(min)
        , max_(max) {}

    iterator begin() const { return iterator(min_); }
    iterator end() const { return iterator(max_); }

    const Integer& min() const { return min_; }
    const Integer& max() const { return max_; }

private:
    Integer min_;
    Integer max_;
};

template<typename Integer>
class CountingRange<Integer>::iterator {
public:
    using value_type = Integer;
    using difference_type = Integer;
    using reference = Integer;
    using pointer = Integer*;
    using iterator_category = std::random_access_iterator_tag;

    iterator()
        : current_() {}

    iterator& operator++() {
        ++current_;
        return *this;
    }

    iterator& operator--() {
        --current_;
        return *this;
    }

    iterator operator++(int) {
        iterator old = *this;
        ++current_;
        return old;
    }

    iterator operator--(int) {
        iterator old = *this;
        --current_;
        return old;
    }

    const Integer& operator*() const { return current_; }

    iterator& operator+=(const Integer& i) {
        current_ += i;
        return *this;
    }

    iterator& operator-=(const Integer& i) {
        current_ -= i;
        return *this;
    }

    Integer operator[](Integer i) { return current_ + i; }

    friend iterator operator+(const iterator& lhs, const Integer& rhs) { return lhs += rhs; }

    friend iterator operator+(const Integer& lhs, const iterator& rhs) { return rhs += lhs; }

    friend iterator operator-(const iterator& lhs, const Integer& rhs) { return lhs -= rhs; }

    friend Integer operator-(const iterator& lhs, const iterator& rhs) { return *lhs - *rhs; }

#define TIRO_FWD_OPERATOR(op) \
    friend bool operator op(const iterator& lhs, const iterator& rhs) { return *lhs op * rhs; }

    TIRO_FWD_OPERATOR(<)
    TIRO_FWD_OPERATOR(>)
    TIRO_FWD_OPERATOR(<=)
    TIRO_FWD_OPERATOR(>=)
    TIRO_FWD_OPERATOR(==)
    TIRO_FWD_OPERATOR(!=)

#undef TIRO_FWD_OPERATOR

private:
    friend CountingRange;

    iterator(const Integer& i)
        : current_(i) {}

private:
    Integer current_;
};

/// Maps a view using a transformation function.
/// Stores a copy of both the view and the function. Iterators
/// handed out by the view refer to the view instance, so it must stay alive
/// for as long as its iterators are being used.
template<typename View, typename Func>
class TransformView {
public:
    class iterator;
    using const_iterator = iterator;

    using value_type = typename iterator::value_type;

    TransformView(View view, Func func)
        : view_(std::move(view))
        , func_(std::move(func)) {}

    iterator begin() const { return iterator(this, view_.begin()); }
    iterator end() const { return iterator(this, view_.end()); }

private:
    View view_;
    Func func_;
};

template<typename View, typename Func>
class TransformView<View, Func>::iterator {
private:
    using inner_iterator = typename View::const_iterator;
    using inner_value = typename std::iterator_traits<inner_iterator>::value_type;

public:
    using iterator_category = std::input_iterator_tag;
    using value_type = decltype(std::declval<const Func&>()(std::declval<const inner_value&>()));
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type;

    iterator()
        : parent_(nullptr)
        , inner_() {}

    iterator& operator++() {
        ++inner();
        return *this;
    }

    iterator operator++(int) {
        auto old = *this;
        ++inner();
        return *this;
    }

    value_type operator*() const {
        TIRO_DEBUG_ASSERT(parent_, "Iterators must be valid when used.");
        return parent_->func_(*inner());
    }

    friend bool operator==(const iterator& lhs, const iterator& rhs) {
        return lhs.parent_ == rhs.parent_ && lhs.inner_ == rhs.inner_;
    }

    friend bool operator!=(const iterator& lhs, const iterator& rhs) { return !(lhs == rhs); }

private:
    friend TransformView;

    iterator(const TransformView* parent, inner_iterator inner)
        : parent_(parent)
        , inner_(std::move(inner)) {}

    inner_iterator& inner() {
        TIRO_DEBUG_ASSERT(parent_, "Iterators must be valid when used.");
        return inner_;
    }

    const inner_iterator& inner() const {
        TIRO_DEBUG_ASSERT(parent_, "Iterators must be valid when used.");
        return inner_;
    }

private:
    const TransformView* parent_;
    inner_iterator inner_;
};

template<typename Range, typename Value>
bool contains(Range&& range, const Value& value) {
    return std::find(std::begin(range), std::end(range), value) != std::end(range);
}

template<typename Range>
std::vector<typename Range::value_type> to_vector(const Range& r) {
    return std::vector<typename Range::value_type>(r.begin(), r.end());
}

} // namespace tiro

#endif // TIRO_COMMON_ITER_TOOLS_HPP
