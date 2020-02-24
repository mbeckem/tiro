#ifndef TIRO_CORE_ID_TYPE
#define TIRO_CORE_ID_TYPE

#include "tiro/core/defs.hpp"
#include "tiro/core/format_stream.hpp"

#include <limits>

namespace tiro {

/// This class is a type safe wrapper that represents a unique id.
/// It is based around a simple underlying integral type.
/// Even though it is just a wrapper, it helps with compile time safety because
/// it makes it impossible to confuse to what type an id belongs to.
///
/// The value `-1` is used as an invalid value.
template<typename Underlying, typename Derived>
class IdType {
public:
    /// The invalid underlying value.
    static constexpr Underlying invalid_value = Underlying(-1);

    /// The invalid id value.
    // TODO: How to make this constexpr?
    static const Derived invalid;

    /// Constructs an invalid id.
    constexpr IdType() = default;

    /// Constructs an invalid id from a nullptr literal, for convenience.
    constexpr IdType(std::nullptr_t) {}

    /// Constructs an id that wraps the provided invalid underlying value.
    constexpr explicit IdType(const Underlying& value)
        : value_(value) {}

    constexpr bool valid() const noexcept { return value_ != invalid_value; }
    constexpr explicit operator bool() const noexcept { return valid(); }

    constexpr const Underlying& value() const noexcept { return value_; }

#define TIRO_ID_COMPARE(op)                       \
    friend constexpr bool operator op(            \
        const Derived& lhs, const Derived& rhs) { \
        return lhs.value() op rhs.value();        \
    }

    TIRO_ID_COMPARE(==)
    TIRO_ID_COMPARE(!=)
    TIRO_ID_COMPARE(<=)
    TIRO_ID_COMPARE(>=)
    TIRO_ID_COMPARE(<)
    TIRO_ID_COMPARE(>)

#undef TIRO_ID_COMPARE

protected:
    void format_name(std::string_view type_name, FormatStream& stream) const {
        if (!valid()) {
            stream.format("{}(invalid)", type_name);
            return;
        }
        stream.format("{}({})", type_name, value());
    }

private:
    Underlying value_ = invalid_value;
};

template<typename Underlying, typename Derived>
const Derived IdType<Underlying, Derived>::invalid{};

#define TIRO_DEFINE_ID(Name, Underlying)                         \
    class Name final : public ::tiro::IdType<Underlying, Name> { \
    public:                                                      \
        using IdType::IdType;                                    \
                                                                 \
        void format(FormatStream& stream) const {                \
            return IdType::format_name(#Name, stream);           \
        }                                                        \
    };

} // namespace tiro

#endif // TIRO_CORE_ID_TYPE
