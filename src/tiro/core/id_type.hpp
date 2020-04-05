#ifndef TIRO_CORE_ID_TYPE
#define TIRO_CORE_ID_TYPE

#include "tiro/core/defs.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"

#include <limits>

namespace tiro {

struct IDTypeBase {};

/// This class is a type safe wrapper that represents a unique id.
/// It is based around a simple underlying integral type.
/// Even though it is just a wrapper, it helps with compile time safety because
/// it makes it impossible to confuse to what type an id belongs to.
///
/// The value `-1` is used as an invalid value.
template<typename Underlying, typename Derived>
class IDType : public IDTypeBase {
public:
    using UnderlyingType = Underlying;

    /// The invalid underlying value.
    static constexpr Underlying invalid_value = Underlying(-1);

    /// The invalid id value.
    // TODO: How to make this constexpr?
    static const Derived invalid;

    /// Constructs an invalid id.
    constexpr IDType() = default;

    /// Constructs an id that wraps the provided invalid underlying value.
    constexpr explicit IDType(const Underlying& value)
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

    void build_hash(Hasher& h) const { h.append(value_); }

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
const Derived IDType<Underlying, Derived>::invalid{};

template<typename ID>
struct IDMapper final {
    static_assert(std::is_base_of_v<IDTypeBase, ID>, "Argument must be a id.");
    static_assert(std::is_final_v<ID>, "Must be concrete id type.");

    using IndexType = typename ID::UnderlyingType;
    using ValueType = ID;

    ValueType to_value(IndexType index) const {
        TIRO_DEBUG_ASSERT(index != ValueType::invalid_value,
            "Cannot map an invalid index to an id.");
        return ValueType(index);
    }

    IndexType to_index(const ValueType& id) const {
        TIRO_DEBUG_ASSERT(id, "Cannot map an invalid id to an index.");
        return id.value();
    }
};

#define TIRO_DEFINE_ID(Name, Underlying)                         \
    class Name final : public ::tiro::IDType<Underlying, Name> { \
    public:                                                      \
        using IDType::IDType;                                    \
                                                                 \
        void format(FormatStream& stream) const {                \
            return IDType::format_name(#Name, stream);           \
        }                                                        \
    };

} // namespace tiro

template<typename T>
struct tiro::EnableBuildHash<T,
    std::enable_if_t<std::is_base_of_v<tiro::IDTypeBase, T>>> : std::true_type {
};

template<typename T>
struct tiro::EnableFormatMember<T,
    std::enable_if_t<std::is_base_of_v<tiro::IDTypeBase, T>>> : std::true_type {
};

#endif // TIRO_CORE_ID_TYPE
