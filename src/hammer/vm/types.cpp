#include "hammer/vm/types.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/hash_table.hpp"

namespace hammer::vm {

void TypeSystem::init(Context& ctx) {
    remove_ = ctx.get_symbol("remove");
}

bool TypeSystem::invoke_member(Context& ctx, Handle<Value> object,
    Symbol member, Span<Value> args, MutableHandle<Value> result) {

    // FIXME demo code
    if (object->is<HashTable>()) {
        Handle<HashTable> table = object.cast<HashTable>();
        if (member.same(remove_)) {
            HAMMER_CHECK(args.size() == 1,
                "HashTable.remove(key) requires one argument ({} given).",
                args.size());
            table->remove(ctx, Handle<Value>::from_slot(&args[0]));
            return true;
        }
    }
    (void) result;
    return false;
}

} // namespace hammer::vm
