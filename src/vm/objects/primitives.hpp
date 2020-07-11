#ifndef TIRO_VM_OBJECTS_PRIMITIVES_HPP
#define TIRO_VM_OBJECTS_PRIMITIVES_HPP

#include "common/math.hpp"
#include "vm/handles/handle.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

/// Represents the null value. All null values have the same representation Value::null().
/// It is just a null pointer under the hood.
class Null final : public Value {
public:
    static Null make(Context& ctx);

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
class Integer final : public HeapValue {
private:
    struct Payload {
        i64 value;
    };

public:
    using Layout = StaticLayout<StaticPayloadPiece<Payload>>;

    static Integer make(Context& ctx, i64 value);

    explicit Integer(Value v)
        : HeapValue(v, DebugCheck<Integer>()) {}

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

    i64 value() const;
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
