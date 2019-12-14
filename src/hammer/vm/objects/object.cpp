#include "hammer/vm/objects/object.hpp"

#include "hammer/vm/context.hpp"

#include "hammer/vm/context.ipp"
#include "hammer/vm/objects/object.ipp"

#include <new>

namespace hammer::vm {

SpecialValue SpecialValue::make(Context& ctx, std::string_view name) {
    // TODO use String as argument type instead for interning.
    Root<String> name_str(ctx, String::make(ctx, name));

    Data* data = ctx.heap().create<Data>(name_str.get());
    return SpecialValue(from_heap(data));
}

std::string_view SpecialValue::name() const {
    return access_heap()->name.view();
}

} // namespace hammer::vm
