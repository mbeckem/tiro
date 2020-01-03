#include "hammer/vm/load.hpp"

#include "hammer/core/overloaded.hpp"
#include "hammer/vm/context.hpp"
#include "hammer/vm/heap/handles.hpp"
#include "hammer/vm/objects/classes.hpp"
#include "hammer/vm/objects/functions.hpp"
#include "hammer/vm/objects/modules.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/strings.hpp"

#include "hammer/vm/context.ipp"

namespace hammer::vm {

static constexpr u32 max_module_size = 1 << 20; // # of members

Module
load_module(Context& ctx, const compiler::CompiledModule& compiled_module,
    const compiler::StringTable& strings) {

    using compiler::ModuleItem;
    using compiler::FunctionDescriptor;

    HAMMER_CHECK(compiled_module.name.valid(),
        "Module definition without a valid module name.");
    HAMMER_CHECK(compiled_module.members.size() <= max_module_size,
        "Module definition is too large.");

    // TODO: Exported members
    Root module_name(
        ctx, ctx.get_interned_string(strings.value(compiled_module.name)));
    Root module_members(ctx, Tuple::make(ctx, compiled_module.members.size()));
    Root<HashTable> module_exported(ctx);
    Root module(
        ctx, Module::make(ctx, module_name, module_members, module_exported));

    u32 index = 0;
    for (const ModuleItem& member : compiled_module.members) {
        Overloaded visitor{//
            [&](const ModuleItem::Function& item) -> Value {
                FunctionDescriptor& f = *item.value;

                Root function_name(
                    ctx, ctx.get_interned_string(
                             f.name ? strings.value(f.name) : "<UNNAMED>"));

                Root tmpl(ctx, FunctionTemplate::make(ctx, function_name,
                                   module, f.params, f.locals, f.code));
                if (f.type == FunctionDescriptor::TEMPLATE)
                    return tmpl;

                Root func(
                    ctx, Function::make(ctx, tmpl, Handle<ClosureContext>()));
                return func;
            },
            [&](const ModuleItem::Integer& item) -> Value {
                return ctx.get_integer(item.value);
            },
            [&](const ModuleItem::Float& item) -> Value {
                return Float::make(ctx, item.value);
            },
            [&](const ModuleItem::String& item) -> Value {
                HAMMER_CHECK(
                    item.value.valid(), "Invalid string in module definition.");
                return ctx.get_interned_string(strings.value(item.value));
            },
            [&](const ModuleItem::Symbol& symbol) -> Value {
                HAMMER_CHECK(symbol.string_index < index,
                    "Symbol string index {} refers to an unprocessed index.",
                    symbol.string_index);

                Root name(ctx, module_members->get(symbol.string_index));
                HAMMER_CHECK(name->is<String>(),
                    "Module member at index {} is not a string.",
                    symbol.string_index);

                return ctx.get_symbol(name.handle().cast<String>());
            },
            [&](const ModuleItem::Import& import) -> Value {
                HAMMER_CHECK(import.string_index < index,
                    "Import string index {} refers to an unprocessed index.",
                    import.string_index);

                Root name(ctx, module_members->get(import.string_index));
                HAMMER_CHECK(name->is<String>(),
                    "Module member at index {} is not a string.",
                    import.string_index);

                Root<Module> imported(ctx);
                if (!ctx.find_module(
                        name.handle().cast<String>(), imported.mut_handle())) {
                    HAMMER_ERROR(
                        "Failed to import module {}: the module was not found.",
                        name->as_strict<String>().view());
                }

                return imported;
            },
            [&](auto &&) -> Value {
                HAMMER_ERROR("Unsupported module member of type {}.",
                    to_string(member.which()));
            }};

        Root value(ctx, visit(member, visitor));
        module_members->set(index, value);
        ++index;
    }
    return module;
}

} // namespace hammer::vm
