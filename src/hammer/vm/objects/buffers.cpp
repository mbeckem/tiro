#include "hammer/vm/objects/buffers.hpp"

#include "hammer/vm/context.hpp"

namespace hammer::vm {

template<typename DataType, typename Derived>
template<typename Init>
Derived BufferBase<DataType, Derived>::make_impl(
    Context& ctx, size_t total_size, Init&& init) {
    size_t allocation_size = variable_allocation<Data, DataType>(total_size);
    Data* data = ctx.heap().create_varsize<Data>(allocation_size, total_size);
    init(data);
    return Derived(Value::from_heap(data));
}

#define HAMMER_BUFFER_FACTORIES(Name, DataType)                           \
    Name Name::make(Context& ctx, size_t size, DataType default_value) {  \
        return BufferBase::make(ctx, size, default_value);                \
    }                                                                     \
                                                                          \
    Name Name::make(Context& ctx, Span<const DataType> content,           \
        size_t total_size, DataType default_value) {                      \
        return BufferBase::make(ctx, content, total_size, default_value); \
    }

HAMMER_BUFFER_TYPES(HAMMER_BUFFER_FACTORIES)
#undef HAMMER_BUFFER_FACTORIES

} // namespace hammer::vm
