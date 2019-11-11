#ifndef HAMMER_VM_OBJECTS_ARRAY_HPP
#define HAMMER_VM_OBJECTS_ARRAY_HPP

#include "hammer/core/span.hpp"
#include "hammer/vm/handles.hpp"
#include "hammer/vm/objects/array_storage_base.hpp"
#include "hammer/vm/objects/value.hpp"

namespace hammer::vm {

class ArrayStorage final : public ArrayStorageBase<Value, ArrayStorage> {
public:
    using ArrayStorageBase::ArrayStorageBase;
};

/**
 * A dynamic, resizable array.
 */
class Array final : public Value {
public:
    static Array make(Context& ctx, size_t initial_capacity);

    static Array make(Context& ctx,
        /* FIXME must be rooted */ Span<const Value> initial_content);

    Array() = default;

    explicit Array(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Array>(), "Value is not an array.");
    }

    size_t size() const;     // Number of values in the array
    size_t capacity() const; // Total capacity before resize is needed
    const Value* data() const;
    Span<const Value> values() const { return {data(), size()}; }

    Value get(size_t index) const;
    void set(Context& ctx, size_t index, Handle<Value> value) const;

    void append(Context& ctx, Handle<Value> value) const;
    void remove_last() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    // Returns size >= required
    static size_t next_capacity(size_t required);

    inline Data* access_heap() const;
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_ARRAY_HPP
