#ifndef TIRO_VM_OBJECTS_NATIVE_OBJECT_HPP
#define TIRO_VM_OBJECTS_NATIVE_OBJECT_HPP

#include "vm/objects/layout.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

class NativeObject final : public HeapValue {
public:
    using FinalizerFn = void (*)(void* data, size_t size);

private:
    struct Payload {
        FinalizerFn cleanup = nullptr;
    };

public:
    using Layout = BufferLayout<byte, alignof(std::max_align_t), StaticPayloadPiece<Payload>>;

    static NativeObject make(Context& ctx, size_t size);

    NativeObject() = default;

    explicit NativeObject(Value v)
        : HeapValue(v, DebugCheck<NativeObject>()) {}

    void* data() const;  // Raw pointer to the native object.
    size_t size() const; // Size of data, in bytes.

    // The function will be executed when the object is collected.
    void finalizer(FinalizerFn cleanup);
    FinalizerFn finalizer() const;

    // Calls the cleanup function. Called by the collector.
    void finalize();

    Layout* layout() const { return access_heap<Layout>(); }
};

/// Wraps a native pointer value. The value is not inspected or owned in any way,
/// the user must make sure that the value remains valid for as long as it is being used.
///
/// Use NativeObject instead if you need more control over the lifetime of native objects.
class NativePointer final : public HeapValue {
private:
    struct Payload {
        void* ptr;
    };

public:
    using Layout = StaticLayout<StaticPayloadPiece<Payload>>;

    static NativePointer make(Context& ctx, void* ptr);

    NativePointer() = default;

    explicit NativePointer(Value v)
        : HeapValue(v, DebugCheck<NativePointer>()) {}

    void* data() const;

    Layout* layout() const { return access_heap<Layout>(); }
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_NATIVE_OBJECT_HPP
