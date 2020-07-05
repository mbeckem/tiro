#include "vm/objects/native_object.hpp"

#include "vm/objects/factory.hpp"

namespace tiro::vm {

NativeObject NativeObject::make(Context& ctx, size_t size) {
    Layout* data = create_object<NativeObject>(ctx, size,
        BufferInit(size, [&](Span<byte> bytes) { std::memset(bytes.begin(), 0, bytes.size()); }),
        StaticPayloadInit());
    return NativeObject(from_heap(data));
}

void* NativeObject::data() const {
    return layout()->buffer_begin();
}

size_t NativeObject::size() const {
    return layout()->buffer_capacity();
}

void NativeObject::finalizer(FinalizerFn cleanup) {
    layout()->static_payload()->cleanup = cleanup;
}

NativeObject::FinalizerFn NativeObject::finalizer() const {
    return layout()->static_payload()->cleanup;
}

void NativeObject::finalize() {
    Layout* data = layout();
    if (data->static_payload()->cleanup) {
        data->static_payload()->cleanup(data->buffer_begin(), data->buffer_capacity());
    }
}

NativePointer NativePointer::make(Context& ctx, void* ptr) {
    Layout* data = create_object<NativePointer>(ctx, StaticPayloadInit());
    data->static_payload()->ptr = ptr;
    return NativePointer(from_heap(data));
}

void* NativePointer::data() const {
    return layout()->static_payload()->ptr;
}

} // namespace tiro::vm
