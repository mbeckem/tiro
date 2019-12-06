#ifndef HAMMER_VM_OBJECTS_CLASSES_HPP
#define HAMMER_VM_OBJECTS_CLASSES_HPP

#include "hammer/vm/objects/value.hpp"

namespace hammer::vm {

/**
 * A method is part of a class and contains a function
 * that can be called with a class instance as the first argument.
 */
class Method final : public Value {
public:
    // TODO Point to the containing class

    static Method make(Context& ctx, Handle<Value> function);

    Method() = default;

    explicit Method(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Method>(), "Value is not a method.");
    }

    Value function() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    inline Data* access_heap() const;
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_CLASSES_HPP
