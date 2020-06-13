#include "vm/modules/module_builder.hpp"

#include "vm/context.hpp"
#include "vm/objects/classes.hpp"
#include "vm/objects/hash_tables.hpp"
#include "vm/objects/strings.hpp"

namespace tiro::vm {

ModuleBuilder::ModuleBuilder(Context& ctx, std::string_view name)
    : ctx_(ctx)
    , name_(ctx)
    , members_(ctx) {
    name_.set(ctx.get_interned_string(name));
    members_.set(HashTable::make(ctx));
}

ModuleBuilder& ModuleBuilder::add_member(std::string_view name, Handle<Value> member) {
    // TODO adjust to changes in the module class once "exported" only maps names to indices.
    Root<Symbol> symbol(ctx_, ctx_.get_symbol(name));

    if (auto found = members_->get(symbol); found) {
        TIRO_ERROR("Module member {} defined twice.", name);
    }

    members_->set(ctx_, symbol.handle(), member);
    return *this;
}

ModuleBuilder& ModuleBuilder::add_function(
    std::string_view name, u32 argc, Handle<Tuple> values, NativeFunction::FunctionType func_ptr) {
    Root<String> func_name(ctx_, ctx_.get_interned_string(name));
    Root<NativeFunction> func(ctx_, NativeFunction::make(ctx_, func_name, values, argc, func_ptr));
    return add_member(name, func.handle());
}

ModuleBuilder& ModuleBuilder::add_async_function(std::string_view name, u32 argc,
    Handle<Tuple> values, NativeAsyncFunction::FunctionType func_ptr) {
    Root<String> func_name(ctx_, ctx_.get_interned_string(name));
    Root<NativeAsyncFunction> func(
        ctx_, NativeAsyncFunction::make(ctx_, func_name, values, argc, func_ptr));
    return add_member(name, func.handle());
}

Module ModuleBuilder::build() {
    // Real values will eventually go here!
    Root<Tuple> members_tuple(ctx_, Tuple::make(ctx_, 0));
    return Module::make(ctx_, name_, members_tuple, members_);
}

} // namespace tiro::vm
