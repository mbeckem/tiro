#ifndef TIRO_COMMON_RANGES_GENERATOR_RANGE_HPP
#define TIRO_COMMON_RANGES_GENERATOR_RANGE_HPP

#include "common/assert.hpp"
#include "common/defs.hpp"
#include "common/type_traits.hpp"

#include <optional>

namespace tiro {

namespace detail {

template<typename GeneratorRetVal>
struct GeneratorRangeValue {
    static_assert(always_false<GeneratorRangeValue>,
        "The generator must return instances of std::optional<T>.");
};

template<typename T>
struct GeneratorRangeValue<std::optional<T>> {
    using value_type = T;
};

} // namespace detail

template<typename Generator>
class GeneratorRange;

template<typename Generator>
class GeneratorRangeIterator {
public:
    using range_type = GeneratorRange<Generator>;
    using iterator_category = std::input_iterator_tag;
    using value_type = typename range_type::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type;

    GeneratorRangeIterator()
        : range_(nullptr) {}

    GeneratorRangeIterator& operator++() {
        TIRO_DEBUG_ASSERT(range_, "Invalid iterator.");
        range_->advance();
        return *this;
    }

    void operator++(int) { this->operator++(); }

    value_type& operator*() const {
        TIRO_DEBUG_ASSERT(range_, "Invalid iterator.");
        TIRO_DEBUG_ASSERT(range_->has_value(), "Cannot dereference past the end iterator.");
        return range_->value();
    }

    friend bool operator==(const GeneratorRangeIterator& lhs, const GeneratorRangeIterator& rhs) {
        return equal(lhs, rhs);
    }

    friend bool operator!=(const GeneratorRangeIterator& lhs, const GeneratorRangeIterator& rhs) {
        return !(lhs == rhs);
    }

private:
    friend range_type;

    GeneratorRangeIterator(range_type& range)
        : range_(&range) {}

    bool is_sentinel() const noexcept { return range_ == nullptr; }

    static bool equal(const GeneratorRangeIterator& lhs, const GeneratorRangeIterator& rhs) {
        if (lhs.is_sentinel())
            return rhs.is_sentinel() || !rhs.range_->has_value();
        if (rhs.is_sentinel())
            return !lhs.range_->has_value();
        return lhs.range_ == rhs.range_;
    }

private:
    range_type* range_;
};

/**
 * A range that produces items by invoking a generator function `g`.
 * `g()` should return an `std::optional<T>`, where an empty optional 
 * signals the end of the range.`T` becomes the value type of the range.
 */
template<typename Generator>
class GeneratorRange {
public:
    using iterator = GeneratorRangeIterator<Generator>;

    using value_type =
        typename detail::GeneratorRangeValue<std::invoke_result_t<Generator>>::value_type;

    GeneratorRange(Generator gen)
        : g_(std::move(gen)) {
        advance();
    }

    iterator begin() { return iterator(*this); }
    iterator end() { return iterator(); }

private:
    friend iterator;

    bool has_value() const { return current_.has_value(); }

    value_type& value() {
        TIRO_DEBUG_ASSERT(has_value(), "generator has no value");
        return *current_;
    }

    void advance() { current_ = g_(); }

private:
    Generator g_;
    std::optional<value_type> current_;
};

} // namespace tiro

#endif // TIRO_COMMON_RANGES_GENERATOR_RANGE_HPP
