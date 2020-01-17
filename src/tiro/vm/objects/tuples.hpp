#ifndef TIRO_VM_OBJECTS_TUPLES_HPP
#define TIRO_VM_OBJECTS_TUPLES_HPP

#include "tiro/core/span.hpp"

#include "tiro/vm/objects/value.hpp"

namespace tiro::vm {

/// A tuple is a sequence of values allocated in a contiguous block on the heap
/// that does not change its size.
class Tuple final : public Value {
public:
    static Tuple make(Context& ctx, size_t size);

    static Tuple
    make(Context& ctx, /* FIXME must be rooted */ Span<const Value> values);

    // total_size must be greater than values.size()
    static Tuple make(Context& ctx,
        /* FIXME must be rooted */ Span<const Value> values, size_t total_size);

    static Tuple
    make(Context& ctx, std::initializer_list<Handle<Value>> values);

public:
    Tuple() = default;

    explicit Tuple(Value v)
        : Value(v) {
        TIRO_ASSERT(v.is<Tuple>(), "Value is not a tuple array.");
    }

    const Value* data() const;
    size_t size() const;
    Span<const Value> values() const { return {data(), size()}; }

    Value get(size_t index) const;
    void set(size_t index, Value value) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    template<typename Init>
    static Tuple make_impl(Context& ctx, size_t total_size, Init&& init);

    inline Data* access_heap() const;
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_TUPLES_HPP
