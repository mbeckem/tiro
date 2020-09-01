#ifndef TIRO_VM_OBJECTS_LAYOUT_HPP
#define TIRO_VM_OBJECTS_LAYOUT_HPP

#include "common/defs.hpp"
#include "common/math.hpp"
#include "common/span.hpp"
#include "vm/handles/traits.hpp"
#include "vm/objects/value.hpp"

#include <new>

namespace tiro::vm {

// This file contains the object layout classes used by heap objects
// to define their layout on the heap. It is not perfect - a finished
// version should be able to represent all possible layouts with a single,
// efficient runtime based instance (one per layout).
// This generalization to a few different layout combinations is a first step.

template<typename LayoutType>
struct LayoutTraits;

// Overflow-checking size computation for array like types.
inline constexpr size_t
safe_array_size(size_t instance_size, size_t element_size, size_t element_count) {
    size_t total = element_size;
    if (!checked_mul(total, element_count))
        throw std::bad_alloc(); // TODO use tiro::Error
    if (!checked_add(total, instance_size))
        throw std::bad_alloc(); // TODO use tiro::Error
    return total;
}

// Unsafe version can be invoked when we already have an object instance (since we know that the computation
// didnt overflow during the allocation).
inline constexpr size_t
unsafe_array_size(size_t instance_size, size_t element_size, size_t element_count) {
    return instance_size + element_size * element_count;
}

struct StaticSlotsInit {};

/// Used to hold a number of normal values (where the count is
/// known at compile time).
/// Layouts that contain this piece will be traced by the garbage collector.
template<size_t SlotCount>
class StaticSlotsPiece {
public:
    explicit StaticSlotsPiece(StaticSlotsInit)
        : slots_() {}

    static constexpr size_t static_slot_count() { return SlotCount; }

    Span<Value*> static_slots() { return Span(slots_, static_slot_count); }

    Value* static_slot(size_t index) {
        TIRO_DEBUG_ASSERT(index < static_slot_count(), "Index out of bounds.");
        return &slots_[index];
    }

    template<typename Type = Value>
    Type read_static_slot(size_t index) {
        return Type(*static_slot(index));
    }

    template<typename Wrapper>
    void write_static_slot(size_t index, Wrapper&& wrapper) {
        *static_slot(index) = unwrap_value(wrapper);
    }

private:
    Value slots_[SlotCount];
};

template<size_t SlotCount>
struct LayoutTraits<StaticSlotsPiece<SlotCount>> {
    using Self = StaticSlotsPiece<SlotCount>;

    static constexpr bool may_contain_references = true;

    template<typename Tracer>
    static void trace(Self* instance, Tracer&& t) {
        for (size_t i = 0; i < SlotCount; ++i)
            t(*instance->static_slot(i));
    }
};

struct StaticPayloadInit {};

/// Used to embed a simple native C++ payload into the layout of an object.
/// The native data must have a trivial destructor and it must be
/// default constructible.
/// This piece will *not* be traced by the garbage collector.
template<typename Payload>
class StaticPayloadPiece {
public:
    using PayloadType = Payload;

    static_assert(std::is_default_constructible_v<Payload>);
    static_assert(std::is_trivially_destructible_v<Payload>);

    explicit StaticPayloadPiece(StaticPayloadInit)
        : payload_() {}

    Payload* static_payload() { return &payload_; }

private:
    Payload payload_;
};

template<typename Payload>
struct LayoutTraits<StaticPayloadPiece<Payload>> {
    using Self = StaticPayloadPiece<Payload>;

    static constexpr bool may_contain_references = false;

    template<typename Tracer>
    static void trace(Self* layout, Tracer&& t) {
        (void) layout;
        (void) t;
    }
};

/// Constructs an object layout of the given pieces. The resulting layout has fixed size.
/// The resulting object will be traced by the garbage collector if one of its pieces
/// needs to be traced (i.e. contains gc values).
template<typename... Pieces>
class StaticLayout final : public Header, public Pieces... {
public:
    template<typename... PiecesInit>
    explicit StaticLayout(Header* type, PiecesInit&&... pieces_init)
        : Header(type)
        , Pieces(std::forward<PiecesInit>(pieces_init))... {}
};

template<typename... Pieces>
struct LayoutTraits<StaticLayout<Pieces...>> {
    using Self = StaticLayout<Pieces...>;

    static constexpr bool may_contain_references =
        (... || LayoutTraits<Pieces>::may_contain_references);

    static constexpr bool has_static_size = true;

    static constexpr size_t static_size = sizeof(Self);

    template<typename Tracer>
    static void trace(Self* instance, Tracer&& t) {
        ((void) LayoutTraits<Pieces>::trace(instance, t), ...);
    }
};

template<typename Func>
struct FixedSlotsInit {
    size_t slot_capacity;
    Func init_slots;

    explicit FixedSlotsInit(size_t capacity, Func init)
        : slot_capacity(capacity)
        , init_slots(init) {}
};

/// Contructs an object layout with an array of trailing values (of runtime size).
/// All slots must be initialized to valid values.
/// The values will be traced by the garbage collector.
template<typename Slot, typename... Pieces>
class FixedSlotsLayout final : public Header, public Pieces... {
public:
    using SlotType = Slot;

    template<typename SlotsInit, typename... PiecesInit>
    explicit FixedSlotsLayout(Header* type, SlotsInit&& slots_init, PiecesInit&&... pieces_init)
        : Header(type)
        , Pieces(std::forward<PiecesInit>(pieces_init))...
        , capacity_(slots_init.slot_capacity) {
        slots_init.init_slots(Span(slots_, capacity_));
    }

    size_t fixed_slot_capacity() { return capacity_; }

    Slot* fixed_slot(size_t index) {
        TIRO_DEBUG_ASSERT(index < capacity_, "Index out of bounds.");
        return &slots_[index];
    }

    Slot* fixed_slots_begin() { return slots_; }

    Slot* fixed_slots_end() { return slots_ + capacity_; }

    Span<Slot> fixed_slots() { return Span(slots_, capacity_); }

private:
    size_t capacity_;
    Slot slots_[];
};

template<typename Slot, typename... Pieces>
struct LayoutTraits<FixedSlotsLayout<Slot, Pieces...>> {
    using Self = FixedSlotsLayout<Slot, Pieces...>;

    static constexpr bool may_contain_references = true;

    static constexpr bool has_static_size = false;

    static size_t dynamic_alloc_size(size_t capacity) {
        return safe_array_size(sizeof(Self), sizeof(Slot), capacity);
    }

    static size_t dynamic_size(Self* instance) {
        return unsafe_array_size(sizeof(Self), sizeof(Slot), instance->fixed_slot_capacity());
    }

    template<typename Tracer>
    static void trace(Self* instance, Tracer&& t) {
        ((void) LayoutTraits<Pieces>::trace(instance, t), ...);
        t(instance->fixed_slots());
    }
};

struct DynamicSlotsInit {
    size_t slot_capacity;

    explicit DynamicSlotsInit(size_t capacity)
        : slot_capacity(capacity) {}
};

/// Constructs an object layout with a resizable array of trailing values (of runtime size).
/// The object layout has a total capacity for `slot_capacity` values, of which exactly `slot_count`
/// values are in use and contain valid data.
/// The first `slot_count` values will be traced by the garbage collector and must be initialized correctly.
template<typename Slot, typename... Pieces>
class DynamicSlotsLayout final : public Header, public Pieces... {
    static_assert(std::is_trivially_destructible_v<Slot>);

public:
    using SlotType = Slot;

    template<typename... PiecesInit>
    explicit DynamicSlotsLayout(
        Header* type, DynamicSlotsInit slots_init, PiecesInit&&... pieces_init)
        : Header(type)
        , Pieces(std::forward<PiecesInit>(pieces_init))...
        , count_(0)
        , capacity_(slots_init.slot_capacity) {}

    size_t dynamic_slot_capacity() { return capacity_; }

    size_t dynamic_slot_count() { return count_; }

    Slot* dynamic_slot(size_t index) {
        TIRO_DEBUG_ASSERT(index < count_, "Index out of bounds.");
        return &slots_[index];
    }

    Slot* dynamic_slots_begin() { return slots_; }

    Slot* dynamic_slots_end() { return slots_ + count_; }

    Span<Slot> dynamic_slots() { return Span(slots_, count_); }

    void add_dynamic_slot(Slot value) {
        TIRO_DEBUG_ASSERT(count_ < capacity_, "Must not be full.");
        new (&slots_[count_]) Slot(value);
        ++count_;
    }

    void add_dynamic_slots(Span<const Slot> values) {
        const size_t n = values.size();
        TIRO_DEBUG_ASSERT(n <= capacity_ - count_, "Must have enough capacity.");
        std::uninitialized_copy_n(values.begin(), n, slots_ + count_);
        count_ += n;
    }

    void remove_dynamic_slot() {
        TIRO_DEBUG_ASSERT(count_ > 0, "Must not be empty.");
        --count_;
    }

    void remove_dynamic_slots(size_t n) {
        TIRO_DEBUG_ASSERT(n <= count_, "Must have at least n elements.");
        count_ -= n;
    }

    void clear_dynamic_slots() { count_ = 0; }

private:
    size_t count_;
    size_t capacity_;
    Slot slots_[];
};

template<typename Slot, typename... Pieces>
struct LayoutTraits<DynamicSlotsLayout<Slot, Pieces...>> {
    using Self = DynamicSlotsLayout<Slot, Pieces...>;

    static constexpr bool may_contain_references = true;

    static constexpr bool has_static_size = false;

    static size_t dynamic_alloc_size(size_t capacity) {
        return safe_array_size(sizeof(Self), sizeof(Slot), capacity);
    }

    static size_t dynamic_size(Self* instance) {
        return unsafe_array_size(sizeof(Self), sizeof(Slot), instance->dynamic_slot_capacity());
    }

    template<typename Tracer>
    static void trace(Self* instance, Tracer&& t) {
        ((void) LayoutTraits<Pieces>::trace(instance, t), ...);
        t(instance->dynamic_slots());
    }
};

template<typename Func>
struct BufferInit {
    size_t capacity;
    Func init;

    explicit BufferInit(size_t capacity_, Func init_)
        : capacity(capacity_)
        , init(init_) {}
};

template<typename DataType, size_t Alignment, typename... Pieces>
class BufferLayout final : public Header, public Pieces... {
    static_assert(std::is_trivially_destructible_v<DataType>);

public:
    template<typename BufferInit, typename... PiecesInit>
    explicit BufferLayout(Header* type, BufferInit&& buffer_init, PiecesInit&&... pieces_init)
        : Header(type)
        , Pieces(std::forward<PiecesInit>(pieces_init))...
        , capacity_(buffer_init.capacity) {
        buffer_init.init(Span(data_, capacity_));
    }

    size_t buffer_capacity() { return capacity_; }

    DataType* buffer_item(size_t index) {
        TIRO_DEBUG_ASSERT(index < capacity_, "Index out of bounds.");
        return &data_[index];
    }

    DataType* buffer_begin() { return data_; }

    DataType* buffer_end() { return data_ + capacity_; }

    Span<DataType> buffer() { return Span(data_, capacity_); }

private:
    size_t capacity_;
    alignas(Alignment) DataType data_[];
};

template<typename DataType, size_t Alignment, typename... Pieces>
struct LayoutTraits<BufferLayout<DataType, Alignment, Pieces...>> {
    using Self = BufferLayout<DataType, Alignment, Pieces...>;

    static constexpr bool may_contain_references =
        (... || LayoutTraits<Pieces>::may_contain_references);

    static constexpr bool has_static_size = false;

    static size_t dynamic_alloc_size(size_t capacity) {
        return safe_array_size(sizeof(Self), sizeof(DataType), capacity);
    }

    static size_t dynamic_size(Self* instance) {
        return unsafe_array_size(sizeof(Self), sizeof(DataType), instance->buffer_capacity());
    }

    template<typename Tracer>
    static void trace(Self* instance, Tracer&& t) {
        ((void) LayoutTraits<Pieces>::trace(instance, t), ...);
    }
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_LAYOUT_HPP
