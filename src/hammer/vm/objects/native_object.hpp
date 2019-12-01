#ifndef HAMMER_VM_OBJECTS_NATIVE_OBJECT_HPP
#define HAMMER_VM_OBJECTS_NATIVE_OBJECT_HPP

#include "hammer/vm/objects/value.hpp"

namespace hammer::vm {

class NativeObject final : public Value {
public:
    using CleanupFn = void (*)(void* data, size_t size);

    static NativeObject make(Context& ctx, size_t size);

    NativeObject() = default;

    explicit NativeObject(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<NativeObject>(), "Value is not a native object.");
    }

    void* data() const;  // Raw pointer to the native object.
    size_t size() const; // Size of data, in bytes.

    // The function will be executed when the object is collected.
    void set_finalizer(CleanupFn cleanup);

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

    // Links the given value into the linked list of finalizers.
    // Called by the collector.
    // FIXME: Not used yet.
    void link_finalizer(Value next);
    Value linked_finalizer() const;

    // Calls the cleanup function. Called by the collector.
    void finalize();

private:
    struct Data;

    inline Data* access_heap() const;
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_NATIVE_OBJECT_HPP
