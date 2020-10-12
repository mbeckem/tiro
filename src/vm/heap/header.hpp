#ifndef TIRO_VM_HEAP_HEADER_HPP
#define TIRO_VM_HEAP_HEADER_HPP

#include "common/defs.hpp"
#include "common/memory/tagged_ptr.hpp"
#include "vm/heap/fwd.hpp"
#include "vm/objects/fwd.hpp"

namespace tiro::vm {

// All pointers allocated through the heap have this many unused bits
// at their end.
inline constexpr size_t heap_align_bits = 2;

// All pointers allocated through the heap allocated to at least this many bytes.
// TODO: The current heap simply uses std::malloc(). We need a custom heap that
// actually guarantees this behaviour. Introduce pages and cells while we're at it..
inline constexpr size_t heap_align = 1 << heap_align_bits;

// Common prefix for all objects on the heap. Currently all heap object layouts
// directly derive from this type.
class Header {
private:
    enum Flags : u32 {
        MarkBit = 0,
        ForwardBit = 1, // Currently unused
    };

    // Contains the type pointer under normal circumstances.
    // TODO: Will also be used for forwarding references with an improved gc.
    TaggedPtr<heap_align_bits> type_field_;

    // FIXME less stupid algorithm (areas of cells; marking bitmaps). This field will be removed
    // with an improved gc.
    Header* next = nullptr;

    bool marked() const { return type_field_.tag_bit<MarkBit>(); }
    void marked(bool mark_bit) { type_field_.tag_bit<MarkBit>(mark_bit); }

    friend Value;
    friend Heap;
    friend ObjectList;
    friend Collector;

public:
    /// Constructs a new header with the given type pointer.
    explicit Header(Header* type)
        : type_field_(type) {}

    struct InvalidTag {};

    Header(InvalidTag) {}

    void type(Header* new_type) { type_field_.pointer(new_type); }
    Header* type() { return static_cast<Header*>(type_field_.pointer()); }
};

/// Returns the size of this heap value, in bytes.
size_t object_size(Header* header);

} // namespace tiro::vm

#endif // TIRO_VM_HEAP_HEADER_HPP
