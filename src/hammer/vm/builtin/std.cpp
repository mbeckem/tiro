#include "hammer/vm/builtin/modules.hpp"

#include "hammer/vm/builtin/module_builder.hpp"
#include "hammer/vm/objects/buffers.hpp"
#include "hammer/vm/objects/classes.hpp"
#include "hammer/vm/objects/strings.hpp"

#include "hammer/vm/context.ipp"
#include "hammer/vm/math.hpp"

#include <cstdio>
#include <limits>

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

static void new_buffer(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();

    size_t size = 0;
    if (auto arg = try_extract_size(frame.arg(0))) {
        size = *arg;
    } else {
        HAMMER_ERROR("Invalid size argument for buffer creation.");
    }

    frame.result(Buffer::make(ctx, size, 0));
}

static void launch(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();
    Handle func = frame.arg(0);
    frame.result(ctx.make_coroutine(func));
}

static void to_utf8(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();
    Handle param = frame.arg(0);
    if (!param->is<String>()) {
        HAMMER_ERROR("to_utf8() requires a string argument.");
    }

    Handle string = param.cast<String>();

    Root<Buffer> buffer(
        ctx, Buffer::make(ctx, string->size(), Buffer::uninitialized));

    // Strings are always utf8 encoded.
    std::copy_n(string->data(), string->size(), buffer->data());
    frame.result(buffer);
}

Module create_std_module(Context& ctx) {
    ModuleBuilder builder(ctx, "std");

    builder.add_function("print", 0, {}, print)
        .add_function("new_string_builder", 0, {}, new_string_builder)
        .add_function("new_object", 0, {}, new_object)
        .add_function("new_buffer", 1, {}, new_buffer)
        .add_function("launch", 1, {}, launch)
        .add_function("to_utf8", 1, {}, to_utf8);
    return builder.build();
}

} // namespace hammer::vm
