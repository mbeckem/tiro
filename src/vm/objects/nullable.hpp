#ifndef TIRO_VM_OBJECTS_NULLABLE_HPP
#define TIRO_VM_OBJECTS_NULLABLE_HPP

#include "common/type_traits.hpp"
#include "vm/objects/fwd.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

/// A value that is either an instance of `T` or null.
/// Note that this is a compile time concept only (its a plain value
/// under the hood).
template<typename T>
class Nullable final : public Value {
public:
    using ValueType = T;

    /// Constructs an instance that holds null.
    Nullable()
        : Value(Value::null()) {}

    Nullable(const Null&)
        : Nullable() {}

    /// Constructs an instance that holds a value. `value` must be a valid `T` or null.
    Nullable(T value)
        : Value(value, DebugCheck<T>()) {}

    /// Constructs an instance that holds a value. `value` must be a valid `T` or null.
    /// Disabled when T == Value (implicit constructor is available then).
    template<typename U = T, std::enable_if_t<!std::is_same_v<U, Value>>* = nullptr>
    explicit Nullable(Value value)
        : Value(value, DebugCheck<Nullable<T>>()) {}

    /// Check whether this instance holds null or a valid value.
    using Value::is_null;
    using Value::operator bool;
    bool has_value() const { return !is_null(); }

    /// Returns the inner value. Fails with an assertion error if this instance is null.
    /// \pre `has_value()`.
    T value() const {
        TIRO_DEBUG_ASSERT(has_value(), "Nullable: instance does not hold a value");
        return T(static_cast<Value>(*this));
    }
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_NULLABLE_HPP
