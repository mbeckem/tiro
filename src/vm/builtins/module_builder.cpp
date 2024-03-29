#include "vm/builtins/module_builder.hpp"

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
    members_index_->set(ctx_, symbol, index).must("failed to add module member");
    return *this;
}

ModuleBuilder&
ModuleBuilder::add_function(std::string_view name, u32 argc, const FunctionPtr& ptr) {
    Scope sc(ctx_);
    Local func_name = sc.local(ctx_.get_interned_string(name));

    auto builder = [&] {
        switch (ptr.type) {
        case FunctionPtrType::Sync:
            return NativeFunction::sync(ptr.sync);
        case FunctionPtrType::Async:
            return NativeFunction::async(ptr.async);
        case FunctionPtrType::Resumable:
            return NativeFunction::resumable(ptr.resumable.func, ptr.resumable.locals);
        }
        TIRO_UNREACHABLE("invalid function pointer type");
    };

    Local func_value = sc.local(builder().name(func_name).params(argc).make(ctx_));
    return add_member(name, func_value);
}

Module ModuleBuilder::build() {
    Scope sc(ctx_);

    const size_t n = members_list_->size();
    Local members_tuple = sc.local(Tuple::make(ctx_, n));
    for (size_t i = 0; i < n; ++i) {
        members_tuple->unchecked_set(i, members_list_->unchecked_get(i));
    }

    Local module = sc.local(Module::make(ctx_, name_, members_tuple, members_index_));
    module->initialized(true);
    return *module;
}

} // namespace tiro::vm
