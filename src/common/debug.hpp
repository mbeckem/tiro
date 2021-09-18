#ifndef TIRO_COMMON_DEBUG_HPP
#define TIRO_COMMON_DEBUG_HPP

namespace tiro {

#if !defined(TIRO_DEBUG) && !defined(NDEBUG)
#    define TIRO_DEBUG 1
#endif

#ifdef TIRO_DEBUG
#    define TIRO_SOURCE_LOCATION() (::tiro::SourceLocation{__FILE__, __LINE__, __func__})
#else
#    define TIRO_SOURCE_LOCATION() (::tiro::source_location_unavailable)
#endif

/// Represents a location in the source code.
/// Similar to std::source_location, which is not available in C++17.
struct SourceLocation {
    // Fields are 0 or NULL if compiled without debug symbols.
    const char* file = nullptr;
    int line = 0;
    const char* function = nullptr;

    constexpr SourceLocation() = default;

    constexpr SourceLocation(const char* file_, int line_, const char* function_)
        : file(file_)
        , line(line_)
        , function(function_) {}

    explicit constexpr operator bool() const { return file != nullptr; }
};

inline constexpr SourceLocation source_location_unavailable{};

} // namespace tiro

#endif // TIRO_COMMON_DEBUG_HPP