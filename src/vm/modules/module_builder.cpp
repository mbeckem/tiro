#include "vm/modules/module_builder.hpp"

#include "vm/context.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/string.hpp"

namespace tiro::vm {

ModuleBuilder::ModuleBuilder(Context& ctx, std::string_view name)
    : ctx_(ctx)
    , sc_(ctx)
    , name_(sc_.local(ctx.get_interned_string(name)))
    , members_list_(sc_.local(Array::make(ctx, 8)))
    , members_index_(sc_.local(HashTable::make(ctx))) {}

ModuleBuilder& ModuleBuilder::add_member(std::string_view name, Handle<Value> member) {
    Scope sc(ctx_);

    Local symbol = sc.local(ctx_.get_symbol(name));
    if (auto found = members_index_->get(*symbol); found) {
        TIRO_ERROR("module member {} defined twice", name);
    }

    Local index = sc.local(ctx_.get_integer(members_list_->size()));
    members_list_->append(ctx_, member).must("failed to add module member");
    members_index_->set(ctx_, symbol, index);
    return *this;
}

ModuleBuilder& ModuleBuilder::add_function(
    std::string_view name, u32 argc, MaybeHandle<Tuple> values, const NativeFunctionStorage& func) {
    Scope sc(ctx_);
    Local func_name = sc.local(ctx_.get_interned_string(name));
    Local func_value = sc.local(NativeFunction::make(ctx_, func_name, values, argc, func));
    return add_member(name, func_value);
}

Module ModuleBuilder::build() {
    Scope sc(ctx_);

    const size_t n = members_list_->size();
    Local members_tuple = sc.local(Tuple::make(ctx_, n));
    for (size_t i = 0; i < n; ++i) {
        members_tuple->set(i, members_list_->get(i));
    }

    Local module = sc.local(Module::make(ctx_, name_, members_tuple, members_index_));
    module->initialized(true);
    return *module;
}

} // namespace tiro::vm
