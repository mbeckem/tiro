#ifndef HAMEMR_VM_OBJECTS_BUFFERS_HPP
#define HAMEMR_VM_OBJECTS_BUFFERS_HPP

#include "common/span.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

class Buffer final : public Value {
public:
    struct Uninitialized {};

    static constexpr auto uninitialized = Uninitialized{};

    static Buffer make(Context& ctx, size_t size, Uninitialized);

    static Buffer make(Context& ctx, size_t size, byte default_value);

    static Buffer
    make(Context& ctx, Span<const byte> content, size_t total_size, byte default_value);

public:
    Buffer() = default;

    explicit Buffer(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<Buffer>(), "Value is not a buffer.");
    }

    byte get(size_t index) const;
    void set(size_t index, byte value) const;

    size_t size() const;
    byte* data() const;
    Span<byte> values() const { return {data(), size()}; }

    inline size_t object_size() const noexcept;

    // does nothing, no references
    template<typename W>
    void walk(W&&) {}

private:
    struct Data;

    inline Data* access_heap() const;

    template<typename Init>
    static Buffer make_impl(Context& ctx, size_t size, Init&& init);
};

} // namespace tiro::vm

#endif // HAMEMR_VM_OBJECTS_BUFFERS_HPP
