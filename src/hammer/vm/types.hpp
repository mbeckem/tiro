#ifndef HAMMER_VM_TYPES_HPP
#define HAMMER_VM_TYPES_HPP

#include "hammer/core/span.hpp"
#include "hammer/vm/fwd.hpp"
#include "hammer/vm/objects/hash_table.hpp"
#include "hammer/vm/objects/object.hpp"

#include <unordered_map>

namespace hammer::vm {

// TODO classes and stuff
class TypeSystem {
public:
    // Called by context
    void init(Context& ctx);

    template<typename W>
    inline void walk(W&& w);

    /**
     * Returns a member function suitable for invokation on the given instance, i.e.
     * `object.member(...)` is valid syntax. Note that, depending on the function
     * returned here, the call must be made in different ways (native functions, this pointer, etc.).
     */
    std::optional<Value> member_function_invokable(
        Context& ctx, Handle<Value> object, Handle<Symbol> member);

private:
    // TODO real datastructure, class objects ...
    std::unordered_map<ValueType, HashTable> classes_;
};

template<typename W>
void TypeSystem::walk(W&& w) {
    for (auto& entry : classes_) {
        auto& members = entry.second;
        w(members);
    }
}

} // namespace hammer::vm

#endif // HAMMER_VM_TYPES_HPP
