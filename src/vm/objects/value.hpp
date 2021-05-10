#ifndef TIRO_VM_VALUE_HPP
#define TIRO_VM_VALUE_HPP

#include "common/adt/not_null.hpp"
#include "common/defs.hpp"
#include "common/type_traits.hpp"
#include "vm/fwd.hpp"
#include "vm/handles/fwd.hpp"
#include "vm/heap/header.hpp"
#include "vm/objects/types.hpp"

#include <string_view>
#include <type_traits>

namespace tiro::vm {

/// Describes the category of a value. Values of different categories
/// usually need different code paths to interpret their internals.
enum class ValueCategory {
    Null,            ///< The value is null
    EmbeddedInteger, ///< The value is an embedded integer
    Heap,            ///< The value lives on the heap
};

/// The uniform representation for all values managed by the VM.
/// A value has pointer size and is either null, or a pointer to some object allocated
/// on the heap, or a small integer (without any indirection).
class Value {
public:
    // This bit is set on the raw value if it contains an embedded integer.
    static constexpr uintptr_t embedded_integer_flag = 1;

    // Number of bits to shift integers by to encode/decode them into uintptr_t values.
    static constexpr uintptr_t embedded_integer_shift = 1;

    // Number of available bits for integer storage.
    static constexpr uintptr_t embedded_integer_bits = CHAR_BIT * sizeof(uintptr_t)
                                                       - embedded_integer_shift;

    /// Produces the null value.
    static constexpr Value null() noexcept { return Value(); }

    /// Same as Value::null().
    constexpr Value() noexcept
        : raw_(0) {}

    /// True if these are the same objects/values.
    bool same(const Value& other) const noexcept { return raw_ == other.raw_; }

    /// Returns true if the value is of the specified type.
    template<typename T>
    inline bool is() const;

    /// Converts this value to the target type.
    /// \pre The type of the value must match the target type.
    template<typename T>
    T must_cast() const {
        TIRO_DEBUG_ASSERT(is<T>(), "Value is not an instance of this type.");
        return T(*this);
    }

    /// Converts the value to the target type, or to null if
    /// the current type does not match the target type.
    template<typename T>
    Nullable<T> try_cast() const {
        if (is<T>()) {
            return Nullable<T>(*this);
        }
        return Nullable<T>();
    }

    /// Returns true if the value is null.
    constexpr bool is_null() const noexcept { return raw_ == 0; }

    /// Returns true if this value contains a pointer to the heap.
    /// Note: the pointer may still be NULL.
    bool is_heap_ptr() const noexcept { return (raw_ & embedded_integer_flag) == 0; }

    /// Returns true if this value contains an embedded integer.
    bool is_embedded_integer() const noexcept { return (raw_ & embedded_integer_flag) != 0; }

    ValueCategory category() const {
        if (is_null())
            return ValueCategory::Null;
        if (is_embedded_integer())
            return ValueCategory::EmbeddedInteger;

        TIRO_DEBUG_ASSERT(
            is_heap_ptr(), "The value must be on the heap if the other conditions are false.");
        return ValueCategory::Heap;
    }

    /// Returns the value type of this value.
    // TODO: Now that all heap values point to their class directly, this should
    // be renamed (to e.g. "builtin type") and used much less frequently.
    ValueType type() const;

    /// Returns true if the value is not null.
    constexpr explicit operator bool() const { return !is_null(); }

    /// Returns the raw representation of this value.
    uintptr_t raw() const noexcept { return raw_; }

protected:
    struct HeapPointerTag {};
    struct EmbeddedIntegerTag {};

    template<typename CheckedType>
    struct DebugCheck {};

    explicit Value(EmbeddedIntegerTag, uintptr_t value)
        : raw_(value) {
        TIRO_DEBUG_ASSERT(
            raw_ & embedded_integer_flag, "Value does not represent an embedded integer.");
    }

    explicit Value(HeapPointerTag, NotNull<Header*> ptr)
        : raw_(reinterpret_cast<uintptr_t>(ptr.get())) {
        TIRO_DEBUG_ASSERT(
            (raw_ & embedded_integer_flag) == 0, "Heap pointer is not aligned correctly.");
    }

    template<typename CheckedType>
    explicit Value(Value v, DebugCheck<CheckedType>)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<CheckedType>(), "Value has unexpected type.");
    }

    static Value from_heap(Header* object) { return Value(HeapPointerTag(), TIRO_NN(object)); }

    static Value from_embedded_integer(uintptr_t raw) { return Value(EmbeddedIntegerTag(), raw); }

private:
    uintptr_t raw_;
};

/// A heap value is a value with dynamically allocated storage on the heap.
/// Every (most derived) heap value type must define a `Layout` typedef and must
/// use instances of that layout for its storage. The garbage collector will
/// inspect that layout and trace it, if necessary.
class HeapValue : public Value {
public:
    explicit HeapValue(Header* header)
        : Value(from_heap(header)) {}

    explicit HeapValue(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is_heap_ptr(), "Value must be a heap pointer.");
    }

    /// Returns the heap pointer stored in this value.
    Header* heap_ptr() const {
        TIRO_DEBUG_ASSERT(is_heap_ptr(), "Value must be a heap pointer.");
        return reinterpret_cast<Header*>(raw());
    }

    /// Returns the internal type instance from this object's header.
    ///
    /// \pre Must not be called during garbage collection since object headers
    /// are reused for temporary storage.
    InternalType type_instance();

protected:
    template<typename CheckedType>
    explicit HeapValue(Value v, DebugCheck<CheckedType> check)
        : Value(v, check) {
        TIRO_DEBUG_ASSERT(v.is_heap_ptr(), "Value must be a heap pointer.");
    }

    // Cast to the inner layout. T must be a layout type derived from Header.
    // Used by derived heap value classes to access their private data.
    // Warning: the type cast in unchecked!
    template<typename T>
    T* access_heap() const {
        static_assert(std::is_base_of_v<Header, T>, "T must be a base class of Header.");
        return static_cast<T*>(heap_ptr());
    }
};

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
        TIRO_DEBUG_ASSERT(has_value(), "Nullable: instance does not holds a value.");
        return T(static_cast<Value>(*this));
    }
};

namespace detail {

template<typename T>
struct is_nullable : std::false_type {};

template<typename T>
struct is_nullable<Nullable<T>> : std::true_type {};

} // namespace detail

template<typename T>
inline constexpr bool is_nullable = detail::is_nullable<remove_cvref_t<T>>::value;

namespace detail {

template<typename T>
struct ValueTypeCheck {
    static bool test(Value v) {
        if constexpr (std::is_same_v<T, Value>) {
            return true;
        } else if constexpr (MapBaseToValueTypes<T>::is_base_type) {
            using base_t = MapBaseToValueTypes<T>;
            auto tag = static_cast<std::underlying_type_t<ValueType>>(v.type());
            return tag >= base_t::min_tag && tag <= base_t::max_tag;
        } else {
            return v.type() == TypeToTag<T>;
        }
    }
};

template<>
struct ValueTypeCheck<HeapValue> {
    static bool test(Value v) { return v.is_heap_ptr(); }
};

template<>
struct ValueTypeCheck<Null> {
    static bool test(Value v) { return v.is_null(); }
};

template<>
struct ValueTypeCheck<SmallInteger> {
    static bool test(Value v) { return v.is_embedded_integer(); }
};

template<>
struct ValueTypeCheck<Number> {
    static bool test(Value v) {
        return ValueTypeCheck<Integer>::test(v) || ValueTypeCheck<Float>::test(v);
    }
};

template<typename T>
struct ValueTypeCheck<Nullable<T>> {
    static bool test(Value v) { return v.is_null() || ValueTypeCheck<T>::test(v); }
};

}; // namespace detail

template<typename T>
bool Value::is() const {
    return detail::ValueTypeCheck<std::remove_cv_t<T>>::test(*this);
}

/// True iff objects of the given type might contain references.
bool may_contain_references(ValueType type);

/// Returns the size of this value on the heap, in bytes.
size_t object_size(Value v);

/// Finalizes the object (calls destructors for native objects).
/// FIXME: A bit in the header or a common base class should indicate
/// which values must be finalized. Only finalizable objects should
/// be visited by the gc for cleanup.
void finalize(Value v);

/// Returns the hash value of `v`.
/// For two values a and b, equal(a, b) implies hash(a) == hash(b).
/// Equal hash values DO NOT imply equality.
size_t hash(Value v);

/// Returns true if a is equal to b, as defined by the language's equality rules.
bool equal(Value a, Value b);

/// Format the value as a string. For debug only.
std::string to_string(Value v);

/// Appends a string representation of the given value to the provided builder.
void to_string(Context& ctx, Handle<StringBuilder> builder, Handle<Value> v);

} // namespace tiro::vm

#endif // TIRO_VM_VALUE_HPP
