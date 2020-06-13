#ifndef TIRO_VM_OBJECTS_PRIMITIVES_HPP
#define TIRO_VM_OBJECTS_PRIMITIVES_HPP

#include "common/math.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

/// Represents the null value. All null values have the same representation Value::null().
/// It is just a null pointer under the hood.
class Null final : public Value {
public:
    static Null make(Context& ctx);

    Null() = default;

    explicit Null(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is_null(), "Value is not null.");
    }

    size_t object_size() const noexcept { return 0; }

    template<typename W>
    void walk(W&&) {}
};

/// Instances of Undefined are used as a sentinel value for uninitialized values.
/// They are never leaked into user code. Accesses that generate an undefined value
/// produce an error instead.
///
/// There is only one instance for each context.
class Undefined final : public Value {
public:
    static Undefined make(Context& ctx);

    Undefined() = default;

    explicit Undefined(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<Undefined>(), "Value is not undefined.");
    }

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/// Instances represent the boolean "true" or "false.
/// The constants true and false are singletons for every context.
class Boolean final : public Value {
public:
    static Boolean make(Context& ctx, bool value);

    Boolean() = default;

    explicit Boolean(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<Boolean>(), "Value is not a boolean.");
    }

    bool value() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/// Represents a heap-allocated 64-bit integer value.
class Integer final : public Value {
public:
    static Integer make(Context& ctx, i64 value);

    Integer() = default;

    explicit Integer(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<Integer>(), "Value is not an integer.");
    }

    i64 value() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
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

    SmallInteger() = default;

    explicit SmallInteger(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<SmallInteger>(), "Value is not a small integer.");
    }

    i64 value() const;

    inline size_t object_size() const noexcept { return 0; }

    template<typename W>
    void walk(W&&) {}
};

/// Represents a heap-allocated 64-bit floating point value.
class Float final : public Value {
public:
    static Float make(Context& ctx, f64 value);

    Float() = default;

    explicit Float(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<Float>(), "Value is not a float.");
    }

    f64 value() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_PRIMITIVES_HPP
