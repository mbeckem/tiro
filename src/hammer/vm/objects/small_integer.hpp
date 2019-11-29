#ifndef HAMMER_VM_OBJECTS_SMALL_INTEGER_HPP
#define HAMMER_VM_OBJECTS_SMALL_INTEGER_HPP

#include "hammer/vm/objects/value.hpp"

namespace hammer::vm {

/**
 * Small integers are integers that can fit into the pointer-representation
 * of a Value object. Instead of allocating the integer on the heap, 
 * it is stored directly in the raw pointer value.
 */
class SmallInteger final : public Value {
private:
    static constexpr uintptr_t available_bits = Value::embedded_integer_bits;

public:
    static constexpr i64 max = (i64(1) << (available_bits - 1)) - 1;
    static constexpr i64 min = -(i64(1) << (available_bits - 1));

    /// Constructs a small integer from the given raw integer value.
    /// \pre value >= min && value <= max.
    static SmallInteger make(i64 value);

    SmallInteger() = default;

    explicit SmallInteger(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<SmallInteger>(), "Value is not a small integer.");
    }

    i64 value() const;

    inline size_t object_size() const noexcept { return 0; }

    template<typename W>
    void walk(W&&) {}
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_SMALL_INTEGER_HPP
