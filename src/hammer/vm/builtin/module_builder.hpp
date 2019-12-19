#ifndef HAMMER_VM_BUILTIN_MODULE_BUILDER_HPP
#define HAMMER_VM_BUILTIN_MODULE_BUILDER_HPP

#include "hammer/vm/heap/handles.hpp"
#include "hammer/vm/objects/functions.hpp"
#include "hammer/vm/objects/modules.hpp"

namespace hammer::vm {

class ModuleBuilder {
public:
    ModuleBuilder(Context& ctx, std::string_view name);

    ModuleBuilder(const ModuleBuilder&) = delete;
    ModuleBuilder& operator=(const ModuleBuilder&) = delete;

    // `name` must stay valid, i.e. not point into the garbage collected heap.
    ModuleBuilder& add_member(std::string_view name, Handle<Value> member);

    // `name` must stay valid, i.e. not point into the garbage collected heap.
    ModuleBuilder& add_function(std::string_view name, u32 argc,
        Handle<Tuple> values, NativeFunction::FunctionType func);

    Module build();

private:
    Context& ctx_;
    Root<String> name_;
    Root<HashTable> members_;
};

}; // namespace hammer::vm

#endif // HAMMER_VM_BUILTIN_MODULE_BUILDER_HPP
