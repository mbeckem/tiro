#ifndef TIRO_VM_MODULES_MODULE_BUILDER_HPP
#define TIRO_VM_MODULES_MODULE_BUILDER_HPP

#include "vm/heap/handles.hpp"
#include "vm/objects/function.hpp"
#include "vm/objects/module.hpp"
#include "vm/objects/native_function.hpp"

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
        std::string_view name, u32 argc, Handle<Tuple> values, NativeFunction::FunctionType func);

    // `name` must stay valid, i.e. not point into the garbage collected heap.
    ModuleBuilder& add_async_function(std::string_view name, u32 argc, Handle<Tuple> values,
        NativeAsyncFunction::FunctionType func);

    Module build();

private:
    Context& ctx_;
    Root<String> name_;
    Root<HashTable> members_;
};

}; // namespace tiro::vm

#endif // TIRO_VM_MODULES_MODULE_BUILDER_HPP
