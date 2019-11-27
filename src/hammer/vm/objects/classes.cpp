#include "hammer/vm/objects/classes.hpp"

#include "hammer/vm/context.hpp"

#include "hammer/vm/objects/classes.ipp"

namespace hammer::vm {

Method Method::make(Context& ctx, Handle<Value> function) {
    Data* data = ctx.heap().create<Data>();
    data->function = function;
    return Method(from_heap(data));
}

Value Method::function() const {
    return access_heap()->function;
}

} // namespace hammer::vm
