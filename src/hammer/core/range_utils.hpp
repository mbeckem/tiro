#ifndef HAMMER_CORE_RANGE_UTILS_HPP
#define HAMMER_CORE_RANGE_UTILS_HPP

#include <algorithm>

namespace hammer {

template<typename Range, typename Value>
bool contains(Range&& range, const Value& value) {
    return std::find(std::begin(range), std::end(range), value)
           != std::end(range);
}

} // namespace hammer

#endif // HAMMER_CORE_RANGE_UTILS_HPP
