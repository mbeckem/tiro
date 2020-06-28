#ifndef TIRO_VM_VALUE_HPP
#define TIRO_VM_VALUE_HPP

#include "common/defs.hpp"
#include "common/type_traits.hpp"
#include "vm/fwd.hpp"
#include "vm/heap/header.hpp"
#include "vm/objects/types.hpp"

#include <string_view>

namespace tiro::vm {

/// The uniform representation for all values managed by the VM.
/// A value has pointer size and contains either a pointer to some object allocated
/// on the heap or a small integer (without any indirection).
class Value {
public:
    static constexpr uintptr_t embedded_integer_flag = 1;

    // Number of bits to shift integers by to encode/decode them into uintptr_t values.
    static constexpr uintptr_t embedded_integer_shift = 1;

    // Number of available bits for integer storage.
    static constexpr uintptr_t embedded_integer_bits = CHAR_BIT * sizeof(uintptr_t)
                                                       - embedded_integer_shift;

    /// Indicates the (intended) absence of a value.
    static constexpr Value null() noexcept { return Value(); }

    /// Returns a value that points to the heap-allocated object.
    /// The object pointer must not be null.
    static Value from_heap(Header* object) {
        TIRO_DEBUG_NOT_NULL(object);
        return Value(HeapPointerTag(), object);
    }

    static Value from_embedded_integer(uintptr_t raw) { return Value(EmbeddedIntegerTag(), raw); }

    /// Same as Value::null().
    constexpr Value() noexcept
        : raw_(0) {}

    /// Returns true if the value is null.
    constexpr bool is_null() const noexcept { return raw_ == 0; }

    /// Returns true if the value is not null.
    constexpr explicit operator bool() const { return !is_null(); }

    /// Returns the value type of this value.
    // TODO: Now that all heap values point to their class directly, this should
    // be renamed (to e.g. "builtin type") and used much less frequently.
    ValueType type() const;

    /// Returns true if the value is of the specified type.
    template<typename T>
    bool is() const {
        if constexpr (std::is_same_v<remove_cvref_t<T>, Value>) {
            return true;
        } else {
            static constexpr ValueType tag = TypeToTag<T>;

            if constexpr (tag == ValueType::Null) {
                return is_null();
            } else if constexpr (tag == ValueType::SmallInteger) {
                return is_embedded_integer();
            } else {
                return !is_null() && is_heap_ptr() && type() == tag;
            }
        }
    }

    /// Casts the object to the given type. This casts propagates null values, i.e.
    /// a cast to some heap type "T" will work if the current type is either "T" or Null.
    /// FIXME remove nulls
    template<typename T>
    T as() const {
        return is_null() ? T() : as_strict<T>();
    }

    /// Like cast, but does not permit null values to propagate. The cast will work only
    /// if the exact type is "T".
    template<typename T>
    T as_strict() const {
        static_assert(sizeof(T) == sizeof(Value), "All derived types must have the same size.");

        TIRO_DEBUG_ASSERT(is<T>(), "Value is not an instance of this type.");
        return T(*this);
    }

    /// Returns the raw representation of this value.
    uintptr_t raw() const noexcept { return raw_; }

    /// Returns true if this value contains a pointer to the heap.
    /// Note: the pointer may still be NULL.
    bool is_heap_ptr() const noexcept { return (raw_ & embedded_integer_flag) == 0; }

    /// Returns true if this value contains an embedded integer.
    bool is_embedded_integer() const noexcept { return (raw_ & embedded_integer_flag) != 0; }

    /// Returns the heap pointer stored in this value.
    /// Requires is_heap_ptr() to be true.
    Header* heap_ptr() const {
        TIRO_DEBUG_ASSERT(is_heap_ptr(), "Raw value is not a heap pointer.");
        return reinterpret_cast<Header*>(raw_);
    }

    /// True if these are the same objects/values.
    bool same(const Value& other) const noexcept { return raw_ == other.raw_; }

protected:
    struct HeapPointerTag {};
    struct EmbeddedIntegerTag {};

    template<typename CheckedType>
    struct DebugCheck {};

    template<typename CheckedType>
    explicit Value(Value v, DebugCheck<CheckedType>)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<CheckedType>(), "Value has unexpected type.");
    }

    explicit Value(EmbeddedIntegerTag, uintptr_t value)
        : raw_(value) {
        TIRO_DEBUG_ASSERT(
            raw_ & embedded_integer_flag, "Value does not represent an embedded integer.");
    }

private:
    explicit Value(HeapPointerTag, Header* ptr)
        : raw_(reinterpret_cast<uintptr_t>(ptr)) {
        TIRO_DEBUG_ASSERT(
            (raw_ & embedded_integer_flag) == 0, "Heap pointer is not aligned correctly.");
    }

private:
    uintptr_t raw_;
};

/// A heap value is a value with dynamically allocated storage on the heap.
/// Every (most derived) heap value type must define a `Layout` typedef and must
/// use instances of that layout for its storage. The garbage collector will
/// inspect that layout and trace it, if necessary.
class HeapValue : public Value {
public:
    // TODO: Remove (Implement nullable types instead).
    HeapValue()
        : Value() {}

protected:
    template<typename CheckedType>
    explicit HeapValue(Value v, DebugCheck<CheckedType> check)
        : Value(v, check) {
        TIRO_DEBUG_ASSERT(v.is_heap_ptr(), "Value must be a heap pointer.");
    }

    // Unchecked cast to the inner layout. T must be a layout type derived from Header.
    // Used by derived heap value classes to access their private data.
    // Warning: the type cast in unchecked!
    template<typename T>
    T* access_heap() const {
        TIRO_DEBUG_ASSERT(is_heap_ptr() && heap_ptr() != nullptr, "Must be a valid heap pointer.");
        static_assert(std::is_base_of_v<Header, T>, "T must be a base class of Header.");
        return static_cast<T*>(heap_ptr());
    }
};

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
