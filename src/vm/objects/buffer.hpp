#ifndef HAMEMR_VM_OBJECTS_BUFFER_HPP
#define HAMEMR_VM_OBJECTS_BUFFER_HPP

#include "common/adt/span.hpp"
#include "vm/handles/handle.hpp"
#include "vm/object_support/fwd.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

class Buffer final : public HeapValue {
public:
    using Layout = BufferLayout<byte, alignof(byte)>;

    struct Uninitialized {};

    static constexpr auto uninitialized = Uninitialized{};

    static Buffer make(Context& ctx, size_t size, Uninitialized);

    static Buffer make(Context& ctx, size_t size, byte default_value);

    static Buffer
    make(Context& ctx, Span<const byte> content, size_t total_size, byte default_value);

    explicit Buffer(Value v)
        : HeapValue(v, DebugCheck<Buffer>()) {}

    /// Returns whether the buffer's address remains stable in memory.
    /// This is currently always the case as the GC does not move objects.
    bool is_pinned() { return true; }

    byte get(size_t index);
    void set(size_t index, byte value);

    size_t size();
    byte* data();
    Span<byte> values() { return {data(), size()}; }

    Layout* layout() const { return access_heap<Layout>(); }

private:
    template<typename Init>
    static Buffer make_impl(Context& ctx, size_t size, Init&& init);
};

extern const TypeDesc buffer_type_desc;

} // namespace tiro::vm

#endif // HAMEMR_VM_OBJECTS_BUFFER_HPP
