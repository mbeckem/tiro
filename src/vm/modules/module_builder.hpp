#ifndef TIRO_VM_MODULES_MODULE_BUILDER_HPP
#define TIRO_VM_MODULES_MODULE_BUILDER_HPP

#include "vm/handles/handle.hpp"
#include "vm/handles/scope.hpp"
#include "vm/objects/function.hpp"
#include "vm/objects/module.hpp"
#include "vm/objects/native.hpp"

namespace tiro::vm {

class ModuleBuilder {
public:
    ModuleBuilder(Context& ctx, std::string_view name);

    ModuleBuilder(const ModuleBuilder&) = delete;
    ModuleBuilder& operator=(const ModuleBuilder&) = delete;

    // `name` must stay valid, i.e. not point into the garbage collected heap.
    ModuleBuilder& add_member(std::string_view name, Handle<Value> member);

    // `name` must stay valid, i.e. not point into the garbage collected heap.
    ModuleBuilder& add_function(
        std::string_view name, u32 argc, MaybeHandle<Tuple> values, const NativeFunctionArg& func);

    Module build();

private:
    Context& ctx_;
    Scope sc_;
    Local<String> name_;
    Local<HashTable> members_;
};

}; // namespace tiro::vm

#endif // TIRO_VM_MODULES_MODULE_BUILDER_HPP
