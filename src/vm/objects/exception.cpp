#include "vm/objects/exception.hpp"

#include "vm/objects/factory.hpp"

namespace tiro::vm {

Exception Exception::make(Context& ctx, Handle<String> message) {
    Layout* data = create_object<Exception>(ctx, StaticSlotsInit());
    data->write_static_slot(MessageSlot, message);
    return Exception(from_heap(data));
}

String Exception::message() {
    return layout()->read_static_slot<String>(MessageSlot);
}

} // namespace tiro::vm
