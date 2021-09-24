#ifndef TIRO_VM_OBJECT_SUPPORT_FACTORY_HPP
#define TIRO_VM_OBJECT_SUPPORT_FACTORY_HPP

#include "common/assert.hpp"
#include "common/defs.hpp"
#include "vm/context.hpp"
#include "vm/object_support/layout.hpp"

namespace tiro::vm {

namespace detail {

template<typename Layout, typename... Args>
inline Layout* create_impl(Heap& heap, Header* type, Args&&... args) {
    using Traits = LayoutTraits<Layout>;
    static_assert(
        Traits::has_static_size, "the layout has dynamic size, use create_varsize instead");
    return heap.template create<Layout>(Traits::static_size, type, std::forward<Args>(args)...);
}

template<typename Layout, typename SizeArg, typename... Args>
inline Layout* create_varsize_impl(Heap& heap, Header* type, SizeArg&& size_arg, Args&&... args) {
    using Traits = LayoutTraits<Layout>;
    static_assert(!Traits::has_static_size, "the layout has static size, use create() instead");

    const size_t total_byte_size = Traits::dynamic_alloc_size(size_arg);

    Layout* data = heap.template create<Layout>(total_byte_size, type, std::forward<Args>(args)...);
    TIRO_DEBUG_ASSERT(Traits::dynamic_size(data) == total_byte_size,
        "byte size mismatch between requested and calculated dynamic object size");
    return data;
}

} // namespace detail

template<typename BuiltinType, typename... LayoutArgs>
auto create_object(Context& ctx, LayoutArgs&&... layout_args) {
    using Layout = typename BuiltinType::Layout;

    auto type = ctx.types().raw_internal_type<BuiltinType>(); // rooted by the TypeSystem instance
    if constexpr (LayoutTraits<Layout>::has_static_size) {
        return detail::create_impl<Layout>(
            ctx.heap(), type, std::forward<LayoutArgs>(layout_args)...);
    } else {
        return detail::create_varsize_impl<Layout>(
            ctx.heap(), type, std::forward<LayoutArgs>(layout_args)...);
    }
}

} // namespace tiro::vm

#endif // TIRO_VM_OBJECT_SUPPORT_FACTORY_HPP
