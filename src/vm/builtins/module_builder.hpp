#ifndef TIRO_VM_BUILTINS_MODULE_BUILDER_HPP
#define TIRO_VM_BUILTINS_MODULE_BUILDER_HPP

#include "vm/handles/handle.hpp"
#include "vm/handles/scope.hpp"
#include "vm/object_support/type_desc.hpp"
#include "vm/objects/function.hpp"
#include "vm/objects/module.hpp"

namespace tiro::vm {

class ModuleBuilder {
public:
    ModuleBuilder(Context& ctx, std::string_view name);

    ModuleBuilder(const ModuleBuilder&) = delete;
    ModuleBuilder& operator=(const ModuleBuilder&) = delete;

    // `name` must stay valid, i.e. not point into the garbage collected heap.
    ModuleBuilder& add_member(std::string_view name, Handle<Value> member);

    // `func.name` must stay valid, i.e. not point into the garbage collected heap.
    ModuleBuilder& add_function(std::string_view name, u32 argc, const FunctionPtr& ptr);

    Module build();

private:
    Context& ctx_;
    Scope sc_;
    Local<String> name_;
    Local<Array> members_list_;
    Local<HashTable> members_index_;
};

}; // namespace tiro::vm

#endif // TIRO_VM_BUILTINS_MODULE_BUILDER_HPP
