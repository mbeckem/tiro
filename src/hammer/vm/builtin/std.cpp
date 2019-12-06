#include "hammer/vm/builtin/std.hpp"

#include "hammer/vm/objects/function.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/string.hpp"

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

Module create_std_module(Context& ctx) {
    Root<Tuple> members(ctx);
    Root<HashTable> exported(ctx, HashTable::make(ctx));
    {
        auto add_member_function = [&](std::string_view name, u32 argc,
                                       NativeFunction::FunctionType func) {
            Root<String> func_name(ctx, ctx.get_interned_string(name));
            Root<NativeFunction> func_obj(ctx,
                NativeFunction::make(ctx, func_name, argc, std::move(func)));
            Root<Symbol> symbol(ctx, ctx.get_symbol(func_name));
            exported->set(ctx, symbol.handle(), func_obj.handle());
        };

        add_member_function("print", 0, print);
        add_member_function("new_string_builder", 0, new_string_builder);
    }

    Root<String> name(ctx, String::make(ctx, "std"));
    Root<Module> module(ctx, Module::make(ctx, name, members, exported));
    return module.get();
}

} // namespace hammer::vm
