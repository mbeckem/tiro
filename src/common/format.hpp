#ifndef TIRO_COMMON_FORMAT_HPP
#define TIRO_COMMON_FORMAT_HPP

#include "common/defs.hpp"

#include <fmt/format.h>

#include <cstdio>
#include <type_traits>

/// Opt into one of the custom formatting interfaces in order to support user defined
/// types in fmt format strings.
///
/// TIRO_ENABLE_MEMBER_FORMAT(Type) will opt into the member function interface, i.e. `object.format(FormatStream&)`.
/// TIRO_ENABLE_FREE_FORMAT(Type) will opt into the free function interface, i.e. `format(object, FormatStream&)`.
/// TIRO_ENABLE_FREE_TO_STRING(Type) will use a free to_string function, i.e. `to_string(object)`.
///
/// The macro invocations should be in the global namespace or in namespace tiro (child namespaces are not
/// supported in C++17, unfortunately).
#define TIRO_ENABLE_MEMBER_FORMAT(...) \
    TIRO_ENABLE_FORMAT_MODE_IMPL(TIRO_SINGLE_ARG(__VA_ARGS__), ::tiro::FormatMode::MemberFormat)
#define TIRO_ENABLE_FREE_FORMAT(...) \
    TIRO_ENABLE_FORMAT_MODE_IMPL(TIRO_SINGLE_ARG(__VA_ARGS__), ::tiro::FormatMode::FreeFormat)
#define TIRO_ENABLE_FREE_TO_STRING(...) \
    TIRO_ENABLE_FORMAT_MODE_IMPL(TIRO_SINGLE_ARG(__VA_ARGS__), ::tiro::FormatMode::FreeToString)

#define TIRO_SINGLE_ARG(...) __VA_ARGS__

#define TIRO_ENABLE_FORMAT_MODE_IMPL(Type, Mode)            \
    template<>                                              \
    struct tiro::EnableFormatMode<Type> {                   \
        static constexpr ::tiro::FormatMode value = (Mode); \
    };

namespace tiro {

enum class FormatMode {
    None,         // Not specialized.
    MemberFormat, // Object has a member function `obj.format(FormatStream&)`.
    FreeFormat,   // There is a free function `format(obj, FormatStream&)`.
    FreeToString, // There is a free function `to_string(obj)` that returns a string / string_view / const char*
};

template<typename T, typename Enable = void>
struct EnableFormatMode {
    static constexpr FormatMode value = FormatMode::None;
};

/// Base class for all format streams.
class FormatStream {
public:
    virtual ~FormatStream();

    FormatStream(const FormatStream&) = delete;
    FormatStream& operator=(const FormatStream&) = delete;

public:
    template<typename... Args>
    FormatStream& format(std::string_view format_str, Args&&... args) {
        do_vformat(format_str, fmt::make_format_args(args...));
        return *this;
    }

    FormatStream& vformat(std::string_view format_str, fmt::format_args args) {
        do_vformat(format_str, args);
        return *this;
    }

protected:
    FormatStream();

    virtual void do_vformat(std::string_view format, fmt::format_args args) = 0;
};

/// A stream that outputs all formatted output into a string.
class StringFormatStream final : public FormatStream {
public:
    explicit StringFormatStream(size_t initial_capacity = 0);
    ~StringFormatStream();

    /// Returns the current output string.
    const std::string& str() const { return buffer_; }

    /// Moves the output string out of the stream. The stream's output buffer will become empty.
    std::string take_str();

private:
    void do_vformat(std::string_view format, fmt::format_args args) override;

private:
    std::string buffer_;
};

/// A stream that appends all formatted output to the given memory buffer.
class BufferFormatStream final : public FormatStream {
public:
    explicit BufferFormatStream(fmt::memory_buffer& buffer);
    ~BufferFormatStream();

    fmt::memory_buffer& buffer() const noexcept { return buffer_; }

private:
    void do_vformat(std::string_view format, fmt::format_args args);

private:
    fmt::memory_buffer& buffer_;
};

/// A stream that appends all formatted output to the given output iterator.
template<typename OutputIterator>
class OutputIteratorStream final : public FormatStream {
public:
    explicit OutputIteratorStream(const OutputIterator& out)
        : out_(out) {}

    ~OutputIteratorStream() = default;

    const OutputIterator& out() const { return out_; }

private:
    void do_vformat(std::string_view format, fmt::format_args args) override {
        fmt::vformat_to(out_, format, args);
    }

private:
    OutputIterator out_;
};

/// A format stream that indents all lines and then prints them to the given base stream.
class IndentStream : public FormatStream {
public:
    explicit IndentStream(FormatStream& base, int indent, bool indent_first = true);
    ~IndentStream();

    FormatStream& base() const;
    int indent() const;

private:
    void do_vformat(std::string_view format, fmt::format_args args) override;

private:
    FormatStream& base_;
    int indent_;
    fmt::memory_buffer buffer_;
    bool indent_next_;
};

/// A format stream that prints directly to the given output file (standard output by default).
class PrintStream : public FormatStream {
public:
    explicit PrintStream();
    explicit PrintStream(std::FILE* out);

    ~PrintStream();

private:
    void do_vformat(std::string_view format, fmt::format_args args) override;

private:
    std::FILE* out_;
};

template<typename T>
struct Repeat final {
    T value;
    size_t count;

    void format(FormatStream& stream) const {
        for (size_t i = 0; i < count; ++i) {
            stream.format("{}", value);
        }
    }
};

template<typename T>
auto repeat(const T& value, size_t count) {
    return Repeat<T>{value, count};
}

inline auto spaces(size_t count) {
    return repeat(' ', count);
}

template<typename T>
struct EnableFormatMode<Repeat<T>> {
    static constexpr FormatMode value = FormatMode::MemberFormat;
};

namespace detail {

template<typename T>
inline constexpr bool has_custom_format = EnableFormatMode<T>::value != FormatMode::None;

template<typename T, typename Stream>
void call_member_format(const T& value, Stream&& stream) {
    value.format(stream);
}

template<typename T, typename Stream>
void call_free_format(const T& value, Stream&& stream) {
    format(value, stream);
}

template<typename T>
auto call_free_to_string(const T& value) {
    return to_string(value);
}

} // namespace detail
} // namespace tiro

template<typename T, typename Char>
struct fmt::formatter<T, Char, std::enable_if_t<tiro::detail::has_custom_format<T>>> {

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const T& value, FormatContext& ctx) {
        constexpr auto mode = tiro::EnableFormatMode<T>::value;
        if constexpr (mode == tiro::FormatMode::MemberFormat) {
            tiro::OutputIteratorStream stream(ctx.out());
            tiro::detail::call_member_format(value, stream);
            return stream.out();
        } else if constexpr (mode == tiro::FormatMode::FreeFormat) {
            tiro::OutputIteratorStream stream(ctx.out());
            tiro::detail::call_free_format(value, stream);
            return stream.out();
        } else {
            return format_to(ctx.out(), "{}", tiro::detail::call_free_to_string(value));
        }
    }
};

#endif // TIRO_COMMON_FORMAT_HPP
