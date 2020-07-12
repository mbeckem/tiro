#include "vm/modules/module_builder.hpp"

#include "vm/context.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/string.hpp"

namespace tiro::vm {

ModuleBuilder::ModuleBuilder(Context& ctx, std::string_view name)
    : ctx_(ctx)
    , sc_(ctx)
    , name_(sc_.local(ctx.get_interned_string(name)))
    , members_(sc_.local(HashTable::make(ctx))) {}

ModuleBuilder& ModuleBuilder::add_member(std::string_view name, Handle<Value> member) {
    // TODO adjust to changes in the module class once "exported" only maps names to indices.
    Scope sc(ctx_);
    Local symbol = sc.local(ctx_.get_symbol(name));

    if (auto found = members_->get(*symbol); found) {
        TIRO_ERROR("Module member {} defined twice.", name);
    }

    members_->set(ctx_, symbol, member);
    return *this;
}

ModuleBuilder& ModuleBuilder::add_function(
    std::string_view name, u32 argc, MaybeHandle<Tuple> values, NativeFunctionPtr func_ptr) {
    Scope sc(ctx_);
    Local func_name = sc.local(ctx_.get_interned_string(name));
    Local func = sc.local(NativeFunction::make(ctx_, func_name, values, argc, func_ptr));
    return add_member(name, func);
}

ModuleBuilder& ModuleBuilder::add_async_function(
    std::string_view name, u32 argc, MaybeHandle<Tuple> values, NativeAsyncFunctionPtr func_ptr) {
    Scope sc(ctx_);
    Local func_name = sc.local(ctx_.get_interned_string(name));
    Local func = sc.local(NativeFunction::make(ctx_, func_name, values, argc, func_ptr));
    return add_member(name, func);
}

Module ModuleBuilder::build() {
    // TODO: Real values will eventually go here!
    Scope sc(ctx_);
    Local members_tuple = sc.local(Tuple::make(ctx_, 0));
    return Module::make(ctx_, name_, members_tuple, members_);
}

} // namespace tiro::vm
