#ifndef TIRO_CORE_RANGE_UTILS_HPP
#define TIRO_CORE_RANGE_UTILS_HPP

#include <algorithm>

namespace tiro {

template<typename Range, typename Value>
bool contains(Range&& range, const Value& value) {
    return std::find(std::begin(range), std::end(range), value)
           != std::end(range);
}

} // namespace tiro

#endif // TIRO_CORE_RANGE_UTILS_HPP
