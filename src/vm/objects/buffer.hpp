#ifndef HAMEMR_VM_OBJECTS_BUFFER_HPP
#define HAMEMR_VM_OBJECTS_BUFFER_HPP

#include "common/span.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/type_desc.hpp"
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

    Buffer() = default;

    explicit Buffer(Value v)
        : HeapValue(v, DebugCheck<Buffer>()) {}

    byte get(size_t index) const;
    void set(size_t index, byte value);

    size_t size() const;
    byte* data() const;
    Span<byte> values() const { return {data(), size()}; }

    Layout* layout() const { return access_heap<Layout>(); }

private:
    template<typename Init>
    static Buffer make_impl(Context& ctx, size_t size, Init&& init);
};

extern const TypeDesc buffer_type_desc;

} // namespace tiro::vm

#endif // HAMEMR_VM_OBJECTS_BUFFER_HPP
