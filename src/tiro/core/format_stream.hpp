#ifndef TIRO_CORE_FORMAT_STREAM_HPP
#define TIRO_CORE_FORMAT_STREAM_HPP

#include "tiro/core/defs.hpp"

#include <fmt/format.h>

#include <type_traits>

/// Opt into one of the custom formatting interfaces in order to support user defined
/// types in fmt format strings.
///
/// TIRO_FORMAT_MEMBER(Type) will opt into the member function interface, i.e. `object.format(FormatStream&)`.
/// TIRO_FORMAT_FREE(Type) will opt into the free function interface, i.e. `format(object, FormatStream&)`.
/// TIRO_FORMAT_FREE_TO_STRING(Type) will use a free to_string function, i.e. `to_string(object)`.
///
/// The macro invocations should be in the global namespace or in namespace tiro (child namespaces are not
/// supported in C++17, unfortunately).
#define TIRO_FORMAT_MEMBER(Type) \
    template<>                   \
    struct tiro::FormatMember<Type> : std::true_type {};
#define TIRO_FORMAT_FREE(Type) \
    template<>                 \
    struct tiro::FormatFree<Type> : std::true_type {};
#define TIRO_FORMAT_FREE_TO_STRING(Type) \
    template<>                           \
    struct tiro::FormatFreeToString<Type> : std::true_type {};

namespace tiro {

template<typename T>
struct FormatMember : std::false_type {};

template<typename T>
struct FormatFree : std::false_type {};

template<typename T>
struct FormatFreeToString : std::false_type {};

class FormatStream {
public:
    virtual ~FormatStream();

    FormatStream(const FormatStream&) = delete;
    FormatStream& operator=(const FormatStream&) = delete;

public:
    template<typename... Args>
    FormatStream& format(std::string_view fmt, Args&&... args) {
        do_vformat(fmt, fmt::make_format_args(args...));
        return *this;
    }

protected:
    FormatStream();

    virtual void do_vformat(std::string_view fmt, fmt::format_args args) = 0;
};

template<typename OutputIterator>
class OutputIteratorStream final : public FormatStream {
public:
    OutputIteratorStream(const OutputIterator& out)
        : out_(out) {}

    ~OutputIteratorStream() = default;

    const OutputIterator& out() const { return out_; }

private:
    void do_vformat(std::string_view fmt, fmt::format_args args) override {
        fmt::vformat_to(out_, fmt, args);
    }

private:
    OutputIterator out_;
};

namespace detail {

template<typename T>
inline constexpr bool has_custom_format = FormatMember<T>::value
                                          || FormatFree<T>::value
                                          || FormatFreeToString<T>::value;

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
struct fmt::formatter<T, Char,
    std::enable_if_t<tiro::detail::has_custom_format<T>>> {

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const T& value, FormatContext& ctx) {
        if constexpr (tiro::FormatMember<T>::value) {
            tiro::OutputIteratorStream stream(ctx.out());
            tiro::detail::call_member_format(value, stream);
            return stream.out();
        } else if constexpr (tiro::FormatFree<T>::value) {
            tiro::OutputIteratorStream stream(ctx.out());
            tiro::detail::call_free_format(value, stream);
            return stream.out();
        } else {
            return format_to(
                ctx.out(), "{}", tiro::detail::call_free_to_string(value));
        }
    }
};

#endif // TIRO_CORE_FORMAT_STREAM_HPP
