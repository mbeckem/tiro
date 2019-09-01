#ifndef HAMMER_CORE_ITER_RANGE_HPP
#define HAMMER_CORE_ITER_RANGE_HPP

#include <utility>

namespace hammer {

/**
 * A pair of iterators that can be iterated using a range based for loop.
 */
template<typename FirstIter, typename SecondIter = FirstIter>
class IterRange {
public:
    IterRange(FirstIter b, SecondIter e)
        : begin_(std::move(b))
        , end_(std::move(e)) {}

    const FirstIter& begin() const { return begin_; }
    const SecondIter& end() const { return end_; }

private:
    FirstIter begin_;
    SecondIter end_;
};

} // namespace hammer

#endif // HAMMER_CORE_ITER_RANGE_HPP
