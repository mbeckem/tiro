#ifndef TIRO_VM_HEAP_HEADER_HPP
#define TIRO_VM_HEAP_HEADER_HPP

#include "common/defs.hpp"
#include "common/memory/tagged_ptr.hpp"
#include "vm/heap/common.hpp"
#include "vm/heap/fwd.hpp"
#include "vm/objects/fwd.hpp"

namespace tiro::vm {

// Common prefix for all objects on the heap. Currently all heap object layouts
// directly derive from this type.
class Header {
private:
    enum Flags : u32 {
        LargeObjectBit = 0,
    };

    // Contains the type pointer under normal circumstances.
    TaggedPtr<cell_align_bits> type_field_;

    bool large_object() const { return type_field_.tag_bit<LargeObjectBit>(); }
    void large_object(bool large_object) { type_field_.tag_bit<LargeObjectBit>(large_object); }

    friend Value;
    friend Heap;
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
