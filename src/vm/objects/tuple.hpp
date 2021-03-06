#ifndef TIRO_VM_OBJECTS_TUPLE_HPP
#define TIRO_VM_OBJECTS_TUPLE_HPP

#include "common/adt/span.hpp"
#include "vm/object_support/fwd.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

/// A tuple is a sequence of values allocated in a contiguous block on the heap
/// that does not change its size.
class Tuple final : public HeapValue {
public:
    using Layout = FixedSlotsLayout<Value>;

    static Tuple make(Context& ctx, size_t size);

    // FIXME: HandleSpac is unsafe here if not used from stack storage. Verify.
    static Tuple make(Context& ctx, HandleSpan<Value> initial_values);

    // size must be greater >= initial_values.size()
    static Tuple make(Context& ctx, HandleSpan<Value> initial_values, size_t size);

    static Tuple make(Context& ctx, std::initializer_list<Handle<Value>> values);

    explicit Tuple(Value v)
        : HeapValue(v, DebugCheck<Tuple>()) {}

    Value* data();
    size_t size();
    Span<Value> values() { return {data(), size()}; }

    Value get(size_t index);
    void set(size_t index, Value value);

    Layout* layout() const { return access_heap<Layout>(); }

private:
    template<typename Init>
    static Tuple make_impl(Context& ctx, size_t total_size, Init&& init);
};

/// Iterates over a tuple.
class TupleIterator final : public HeapValue {
private:
    enum Slots {
        TupleSlot,
        SlotCount_,
    };

    struct Payload {
        size_t index = 0;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static TupleIterator make(Context& ctx, Handle<Tuple> tuple);

    explicit TupleIterator(Value v)
        : HeapValue(v, DebugCheck<TupleIterator>()) {}

    std::optional<Value> next();

    Layout* layout() const { return access_heap<Layout>(); }
};

extern const TypeDesc tuple_type_desc;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_TUPLE_HPP
