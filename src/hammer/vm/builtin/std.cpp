#include "hammer/vm/builtin/modules.hpp"

#include "hammer/vm/builtin/module_builder.hpp"
#include "hammer/vm/objects/classes.hpp"
#include "hammer/vm/objects/strings.hpp"

#include "hammer/vm/context.ipp"

#include <cstdio>

namespace hammer::vm {

static void print(NativeFunction::Frame& frame) {
    const size_t args = frame.arg_count();

    Context& ctx = frame.ctx();
    Root<StringBuilder> builder(ctx, StringBuilder::make(ctx));
    for (size_t i = 0; i < args; ++i) {
        if (i != 0) {
            builder->append(ctx, " ");
        }
        to_string(ctx, builder.handle(), frame.arg(i));
    }
    builder->append(ctx, "\n");

    // TODO stdout from ctx
    std::string_view message = builder->view();
    std::fwrite(message.data(), 1, message.size(), stdout);
    std::fflush(stdout);
}

static void new_string_builder(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();
    frame.result(StringBuilder::make(ctx));
}

static void new_object(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();
    frame.result(DynamicObject::make(ctx));
}

Module create_std_module(Context& ctx) {
    ModuleBuilder builder(ctx, "std");

    builder.add_function("print", 0, print)
        .add_function("new_string_builder", 0, new_string_builder)
        .add_function("new_object", 0, new_object);
    return builder.build();
}

} // namespace hammer::vm
