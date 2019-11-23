#include "hammer/vm/types.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/hash_table.hpp"
#include "hammer/vm/objects/string.hpp"

namespace hammer::vm {

static HashTable hashtable_class(Context& ctx) {
    Root<HashTable> table(ctx, HashTable::make(ctx));

    const auto wrap_table = [](auto fn) {
        return [fn = std::move(fn)](NativeFunction::Frame& frame) {
            Handle<Value> value = frame.arg(0);
            if (!value->is<HashTable>()) {
                // TODO generic
                HAMMER_ERROR("`this` is not a hash table.");
            }
            return fn(value.cast<HashTable>(), frame);
        };
    };

    const auto set = [](Handle<HashTable> self, NativeFunction::Frame& frame) {
        self->set(frame.ctx(), frame.arg(1), frame.arg(2));
    };

    const auto contains = [](Handle<HashTable> self,
                              NativeFunction::Frame& frame) {
        bool result = self->contains(frame.arg(1));
        frame.result(frame.ctx().get_boolean(result));
    };

    const auto remove = [](Handle<HashTable> self,
                            NativeFunction::Frame& frame) {
        self->remove(frame.ctx(), frame.arg(1));
    };

    const auto add_member = [&](std::string_view name, auto fn, u32 argc) {
        Root<Symbol> member(ctx, ctx.get_symbol(name));
        Root<String> member_str(ctx, member->name());
        Root<NativeFunction> func(ctx,
            NativeFunction::make_method(ctx, member_str, argc, wrap_table(fn)));
        table->set(ctx, member.handle(), func.handle());
    };

    add_member("set", set, 3);
    add_member("contains", contains, 2);
    add_member("remove", remove, 2);
    return table.get();
}

void TypeSystem::init([[maybe_unused]] Context& ctx) {
    classes_.emplace(ValueType::HashTable, hashtable_class(ctx));
}

std::optional<Value>
TypeSystem::member_function_invokable([[maybe_unused]] Context& ctx,
    Handle<Value> object, Handle<Symbol> member) {
    if (object->is<Module>()) {
        Handle<Module> module = object.cast<Module>();
        return module->exported().get(member.get());
    } else {
        auto class_pos = classes_.find(object->type());
        if (class_pos == classes_.end())
            return {};

        auto members = Handle<HashTable>::from_slot(&class_pos->second);
        return members->get(member.get());
    }
}

} // namespace hammer::vm
