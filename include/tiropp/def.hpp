#ifndef TIROPP_DEF_HPP_INCLUDED
#define TIROPP_DEF_HPP_INCLUDED

#include <cstddef>
#include <cstdint>

#if !defined(NDEBUG) && !defined(TIRO_DEBUG)
#    define TIRO_DEBUG
#endif

// Handle checks are enabled in debug mode by default.
// Define TIRO_HANDLE_CHECKS to force checks even in release builds.
#if defined(TIRO_DEBUG) && !defined(TIRO_HANDLE_CHECKS)
#    define TIRO_HANDLE_CHECKS
#endif

// Define TIRO_ASSERT(expr) to force enable assertions
#if !defined(TIRO_ASSERT)
#    if defined(TIRO_DEBUG)
#        include <cassert>
#        define TIRO_ASSERT(expr) assert(expr)
#    else
#        define TIRO_ASSERT(expr)
#    endif
#endif

namespace tiro {

using std::int8_t;
using std::int32_t;
using std::int64_t;
using std::int16_t;

using std::uint8_t;
using std::uint32_t;
using std::uint64_t;
using std::uint16_t;

using std::size_t;

} // namespace tiro

#endif // TIROPP_DEF_HPP_INCLUDED
