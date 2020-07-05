#ifndef TIRO_VM_OBJECTS_FACTORY_HPP
#define TIRO_VM_OBJECTS_FACTORY_HPP

#include "common/defs.hpp"
#include "vm/context.hpp"
#include "vm/objects/layout.hpp"

namespace tiro::vm {

namespace detail {

template<typename Layout, typename... LayoutArgs>
auto create_object_static(Context& ctx, Header* type, LayoutArgs&&... layout_args) {
    return ctx.heap().create<Layout>(type, std::forward<LayoutArgs>(layout_args)...);
}

template<typename Layout, typename SizeArg, typename... LayoutArgs>
auto create_object_varsize(
    Context& ctx, Header* type, SizeArg&& size_arg, LayoutArgs&&... layout_args) {
    using Traits = LayoutTraits<Layout>;
    const size_t allocation_size = Traits::dynamic_size(size_arg);

    auto instance = ctx.heap().create_varsize<Layout>(
        allocation_size, type, std::forward<LayoutArgs>(layout_args)...);
    TIRO_DEBUG_ASSERT(Traits::dynamic_size(instance) == allocation_size,
        "Variable size object must report exactly the requested size.");
    return instance;
}

} // namespace detail

template<typename BuiltinType, typename... LayoutArgs>
auto create_object(Context& ctx, LayoutArgs&&... layout_args) {
    using Layout = typename BuiltinType::Layout;

    auto type = ctx.types().internal_type<BuiltinType>(); // rooted by the TypeSystem instance
    if constexpr (LayoutTraits<Layout>::has_static_size) {
        return detail::create_object_static<Layout>(
            ctx, type, std::forward<LayoutArgs>(layout_args)...);
    } else {
        return detail::create_object_varsize<Layout>(
            ctx, type, std::forward<LayoutArgs>(layout_args)...);
    }
}

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_FACTORY_HPP
