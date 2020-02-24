#ifndef TIRO_CORE_ITER_RANGE_HPP
#define TIRO_CORE_ITER_RANGE_HPP

#include <iterator>
#include <utility>

namespace tiro {

///A pair of iterators that can be iterated using a range based for loop.
template<typename FirstIter, typename SecondIter = FirstIter>
class IterRange {
public:
    using value_type = typename std::iterator_traits<FirstIter>::value_type;

    IterRange(FirstIter b, SecondIter e)
        : begin_(std::move(b))
        , end_(std::move(e)) {}

    const FirstIter& begin() const { return begin_; }
    const SecondIter& end() const { return end_; }

private:
    FirstIter begin_;
    SecondIter end_;
};

} // namespace tiro

#endif // TIRO_CORE_ITER_RANGE_HPP
