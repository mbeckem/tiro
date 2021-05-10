#ifndef TIRO_VM_OBJECTS_PRIMITIVES_HPP
#define TIRO_VM_OBJECTS_PRIMITIVES_HPP

#include "common/math.hpp"
#include "vm/handles/handle.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

/// Represents the null value. All null values have the same representation Value::null().
/// It is just a null pointer under the hood.
class Null final : public Value {
public:
    Null() = default;

    explicit Null(Value v)
        : Value(v, DebugCheck<Null>()) {}
};

/// Instances of Undefined are used as a sentinel value for uninitialized values.
/// They are never leaked into user code. Accesses that generate an undefined value
/// produce an error instead.
///
/// There is only one instance for each context.
class Undefined final : public HeapValue {
public:
    using Layout = StaticLayout<>;

    static Undefined make(Context& ctx);

    explicit Undefined(Value v)
        : HeapValue(v, DebugCheck<Undefined>()) {}

    Layout* layout() { return access_heap<Layout>(); }
};

/// Instances represent the boolean "true" or "false.
/// The constants true and false are singletons for every context.
class Boolean final : public HeapValue {
private:
    struct Payload {
        bool value;
    };

public:
    using Layout = StaticLayout<StaticPayloadPiece<Payload>>;

    static Boolean make(Context& ctx, bool value);

    explicit Boolean(Value v)
        : HeapValue(v, DebugCheck<Boolean>()) {}

    bool value();

    Layout* layout() { return access_heap<Layout>(); }
};

/// Represents a heap-allocated 64-bit integer value.
class HeapInteger final : public HeapValue {
private:
    struct Payload {
        i64 value;
    };

public:
    using Layout = StaticLayout<StaticPayloadPiece<Payload>>;

    static HeapInteger make(Context& ctx, i64 value);

    explicit HeapInteger(Value v)
        : HeapValue(v, DebugCheck<HeapInteger>()) {}

    i64 value();

    Layout* layout() { return access_heap<Layout>(); }
};

/// Small integers are integers that can fit into the pointer-representation
/// of a Value object. Instead of allocating the integer on the heap,
/// it is stored directly in the raw pointer value.
class SmallInteger final : public Value {
private:
    static constexpr uintptr_t available_bits = Value::embedded_integer_bits;

public:
    static constexpr i64 max = (i64(1) << (available_bits - 1)) - 1;
    static constexpr i64 min = -(i64(1) << (available_bits - 1));

    static bool fits(i64 value) { return value >= min && value <= max; }

    /// Constructs a small integer from the given raw integer value.
    /// \pre value >= min && value <= max.
    static SmallInteger make(i64 value);

    explicit SmallInteger(Value v)
        : Value(v, DebugCheck<SmallInteger>()) {}

    i64 value();
};

/// Represents an integer with arbitrary storage mode (small integer or heap integer).
class Integer final : public Value {
private:
    template<typename Func>
    auto dispatch(Func&&);

public:
    static std::optional<i64> try_extract(Value v) {
        if (!v.is<Integer>()) {
            return {};
        }
        return static_cast<Integer>(v).value();
    }

    static std::optional<size_t> try_extract_size(Value v) {
        if (!v.is<Integer>()) {
            return {};
        }
        return static_cast<Integer>(v).try_extract_size();
    }

    explicit Integer(Value v)
        : Value(v, DebugCheck<Integer>()) {}

    Integer(SmallInteger v)
        : Integer(static_cast<Value>(v)) {}

    Integer(HeapInteger v)
        : Integer(static_cast<Value>(v)) {}

    /// Returns the value stored in this integer.
    i64 value();

    /// Attempts to extract a valid `size_t` value from this integer.
    /// Returns an empty optional if this integer is not in bounds.
    std::optional<size_t> try_extract_size();
};

/// Represents a heap-allocated 64-bit floating point value.
class Float final : public HeapValue {
private:
    struct Payload {
        f64 value;
    };

public:
    using Layout = StaticLayout<StaticPayloadPiece<Payload>>;

    static Float make(Context& ctx, f64 value);

    explicit Float(Value v)
        : HeapValue(v, DebugCheck<Float>()) {}

    f64 value();

    Layout* layout() { return access_heap<Layout>(); }
};

/// Represents an arbitrary number.
class Number final : public Value {
private:
    template<typename Func>
    auto dispatch(Func&&);

public:
    static std::optional<i64> try_extract_int(Value v) {
        if (!v.is<Number>()) {
            return {};
        }
        return static_cast<Number>(v).try_extract_int();
    }

    static std::optional<size_t> try_extract_size(Value v) {
        if (!v.is<Number>()) {
            return {};
        }
        return static_cast<Number>(v).try_extract_size();
    }

    explicit Number(Value v)
        : Value(v, DebugCheck<Number>()) {}

    Number(Integer v)
        : Number(static_cast<Value>(v)) {}

    Number(Float v)
        : Number(static_cast<Value>(v)) {}

    /// Returns the value of this number converted to float.
    /// May lose precision.
    f64 convert_float();

    /// Returns the value of this number converted to an integer.
    /// Fractional parts will be truncated.
    i64 convert_int();

    /// Attempts to extract an integer value from this number.
    /// Fails (with an empty optional) if this number represents a floating point value.
    ///
    /// TODO: Should this function extract integers from floats that do not have a fractional part?
    std::optional<i64> try_extract_int();

    /// Attempts to extract a valid `size_t` value from this integer.
    /// Returns an empty optional if this integer is not in bounds.
    ///
    /// TODO: Should this function extract integers from floats that do not have a fractional part?
    std::optional<size_t> try_extract_size();
};

class Symbol final : public HeapValue {
public:
    using Layout = StaticLayout<StaticSlotsPiece<1>>;

    enum { NameSlot };

    /// String must be interned.
    static Symbol make(Context& ctx, Handle<String> name);

    explicit Symbol(Value v)
        : HeapValue(v, DebugCheck<Symbol>()) {}

    String name();

    bool equal(Symbol other);

    Layout* layout() const { return access_heap<Layout>(); }
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_PRIMITIVES_HPP
