#ifndef HAMMER_VM_OBJECTS_MODULES_HPP
#define HAMMER_VM_OBJECTS_MODULES_HPP

#include "hammer/vm/objects/value.hpp"

namespace hammer::vm {

/// Represents a module, which is a collection of exported and private members.
class Module final : public Value {
public:
    static Module make(Context& ctx, Handle<String> name, Handle<Tuple> members,
        Handle<HashTable> exported);

    Module() = default;

    explicit Module(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Module>(), "Value is not a module.");
    }

    String name() const;
    Tuple members() const;
    HashTable exported() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_MODULES_HPP
