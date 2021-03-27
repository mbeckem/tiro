#ifndef TIRO_COMMON_ENTITIES_ENTITY_ID_HPP
#define TIRO_COMMON_ENTITIES_ENTITY_ID_HPP

#include "common/assert.hpp"
#include "common/defs.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"

#include <limits>

namespace tiro {

struct EntityIdBase {};

/// This class is a type safe wrapper that represents a unique entity id.
/// It is based around a simple underlying integral type.
/// Even though it is just a wrapper, it helps with compile time safety because
/// it makes it impossible to confuse to what type an id belongs to.
///
/// The value `-1`, casted to the underlying type, is used as an invalid value.
template<typename Underlying, typename Derived>
class EntityId : public EntityIdBase {
public:
    using UnderlyingType = Underlying;

    /// The invalid underlying value.
    static constexpr Underlying invalid_value = Underlying(-1);

    /// The invalid id value.
    static const Derived invalid;

    /// Constructs an invalid id.
    constexpr EntityId() = default;

    /// Constructs an id that wraps the provided invalid underlying value.
    constexpr explicit EntityId(const Underlying& value)
        : value_(value) {}

    constexpr bool valid() const noexcept { return value_ != invalid_value; }
    constexpr explicit operator bool() const noexcept { return valid(); }

    constexpr const Underlying& value() const noexcept { return value_; }

#define TIRO_COMPARE_IMPL(op)                                                   \
    friend constexpr bool operator op(const Derived& lhs, const Derived& rhs) { \
        return lhs.value() op rhs.value();                                      \
    }

    TIRO_COMPARE_IMPL(==)
    TIRO_COMPARE_IMPL(!=)
    TIRO_COMPARE_IMPL(<=)
    TIRO_COMPARE_IMPL(>=)
    TIRO_COMPARE_IMPL(<)
    TIRO_COMPARE_IMPL(>)

#undef TIRO_COMPARE_IMPL

    void hash(Hasher& h) const { h.append(value_); }

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
const Derived EntityId<Underlying, Derived>::invalid{};

#define TIRO_DEFINE_ENTITY_ID(Name, Underlying)                                                  \
    class Name final : public ::tiro::EntityId<Underlying, Name> {                               \
    public:                                                                                      \
        using EntityId::EntityId;                                                                \
                                                                                                 \
        void format(FormatStream& stream) const { return EntityId::format_name(#Name, stream); } \
    };

} // namespace tiro

template<typename T>
struct tiro::EnableMemberHash<T, std::enable_if_t<std::is_base_of_v<tiro::EntityIdBase, T>>>
    : std::true_type {};

template<typename T>
struct tiro::EnableFormatMode<T, std::enable_if_t<std::is_base_of_v<tiro::EntityIdBase, T>>> {
    static constexpr tiro::FormatMode value = tiro::FormatMode::MemberFormat;
};

#endif // TIRO_COMMON_ENTITIES_ENTITY_ID_HPP
