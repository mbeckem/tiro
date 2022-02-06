#include "vm/root_set.hpp"

#include "vm/objects/hash_table.hpp"
#include "vm/objects/primitives.hpp"
#include "vm/objects/set.hpp"

namespace tiro::vm {

RootSet::RootSet() {}

RootSet::~RootSet() {}

void RootSet::init(Context& ctx) {
    interpreter_.init(ctx);
    types_.init_internal(ctx);

    true_ = Boolean::make(ctx, true);
    false_ = Boolean::make(ctx, false);
    undefined_ = Undefined::make(ctx);
    interned_strings_ = HashTable::make(ctx);

    modules_.init(ctx);
    types_.init_public(ctx);
}

Handle<Boolean> RootSet::get_true() {
    return Handle(&true_).must_cast<Boolean>();
}

Handle<Boolean> RootSet::get_false() {
    return Handle(&false_).must_cast<Boolean>();
}

Handle<Undefined> RootSet::get_undefined() {
    return Handle(&undefined_).must_cast<Undefined>();
}

Handle<HashTable> RootSet::get_interned_strings() {
    return Handle(&interned_strings_).must_cast<HashTable>();
}

MutHandle<Nullable<Coroutine>> RootSet::get_first_ready() {
    return MutHandle(&first_ready_);
}

MutHandle<Nullable<Coroutine>> RootSet::get_last_ready() {
    return MutHandle(&last_ready_);
}

} // namespace tiro::vm
