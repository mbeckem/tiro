#include "hammer/vm/objects/native_object.hpp"

#include "hammer/vm/context.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/native_object.ipp"

namespace hammer::vm {

NativeObject NativeObject::make(Context& ctx, size_t size) {
    size_t total_size = variable_allocation<Data, byte>(size);
    Data* data = ctx.heap().create_varsize<Data>(total_size, size);
    std::memset(data->data, 0, size);
    return NativeObject(from_heap(data));
}

void* NativeObject::data() const {
    return access_heap()->data;
}

size_t NativeObject::size() const {
    return access_heap()->size;
}

void NativeObject::set_finalizer(CleanupFn cleanup) {
    access_heap()->cleanup = cleanup;
}

void NativeObject::link_finalizer(Value next) {
    access_heap()->next_finalizer = next;
}

Value NativeObject::linked_finalizer() const {
    return access_heap()->next_finalizer;
}

void NativeObject::finalize() {
    Data* d = access_heap();
    if (d->cleanup) {
        d->cleanup(d->data, d->size);
    }
}

} // namespace hammer::vm
