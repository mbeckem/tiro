#ifndef HAMMER_VM_HANDLES_HPP
#define HAMMER_VM_HANDLES_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/type_traits.hpp"
#include "hammer/vm/value.hpp"

#include <utility>

namespace hammer::vm {

class Context;

template<typename T>
class Handle;

template<typename T>
class MutableHandle;

class RootBase;

template<typename T>
class Root;

class RootBase {
protected:
    RootBase(Context& ctx, Value value);

    ~RootBase();

    RootBase(const RootBase&) = delete;
    RootBase& operator=(const RootBase&) = delete;

private:
    // Implements an intrusive stack of rooted base instances.
    // The gc will walk this stack and mark all reachable values as alive.
    // We could switch to a v8-like HandleScope (or rather RootedScope) approach
    // to avoid the many stack operations (one push / pop for every Root instance).
    // Locality of values on the stack is not ideal when scanning, since every Root object has its
    // own value and we need pointer chasing to find the next one.

    // We pushed ourselves onto the stack.
    RootBase** stack_ = nullptr;

    // The previous "top" pointer.
    RootBase* prev_ = nullptr;

    friend Context;

protected:
    Value slot_;
};

template<typename T, typename Derived>
class PointerOps {
public:
    struct Holder {
        T value;

        // Both branches are safe because the Holder lives until the end of the full
        // expression (i.e. the ";").
        auto operator-> () {
            if constexpr (std::is_pointer_v<T>) {
                HAMMER_ASSERT_NOT_NULL(value);
                return value;
            } else {
                return &value;
            }
        }
    };

    Holder operator->() { return {self()->get()}; }
    T operator*() { return self()->get(); }

protected:
    const Derived* self() const { return static_cast<const Derived*>(this); }
    Derived* self() { return static_cast<Derived*>(this); }
};

// TODO nullable?
template<typename T>
class Root final : public RootBase, public PointerOps<T, Root<T>> {
public:
    Root(Context& ctx)
        : Root(ctx, T()) {}

    Root(Context& ctx, T initial_value)
        : RootBase(ctx, initial_value) {}

    T get() { return slot_.as<T>(); }
    void set(T value) { slot_ = value; }
    /* implicit */ operator T() { return get(); }
    /* implicit */ operator Value() { return slot_; }

    inline Handle<T> handle();
    inline MutableHandle<T> mut_handle();

    /* implicit */ operator Handle<T>() { return handle(); }
};

class HandleBase {
protected:
    inline static const Value null_value_ = Value::null();
};

/**
 * A handle refers to an object that is rooted somewhere else,
 * and is thus guaranteed to survive a garbage collection cycle.
 * Handles should be used as function input arguments.
 * 
 * A handle must not be used when it is not rooted anymore (e.g. because
 * the original Rooted object was destroyed).
 */
template<typename T>
class Handle final : public HandleBase, public PointerOps<T, Handle<T>> {
public:
    static Handle from_slot(Value* slot) {
        // FIXME type assertion
        return Handle(slot);
    }

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>>* = nullptr>
    /* implicit */ Handle(Handle<U> other)
        : slot_(other.slot_) {
        // Triggers an error if invalid type.
        HAMMER_ASSERT((slot_->as<T>(), true), "");
    }

    /* implicit */ Handle()
        : slot_(&null_value_) {}

    T get() const { return slot_->as<T>(); }
    /* implicit */ operator T() const { return get(); }
    /* implicit */ operator Value() { return *slot_; }

private:
    explicit Handle(Value* slot)
        : slot_(slot) {
        HAMMER_ASSERT_NOT_NULL(slot);
    }

private:
    const Value* slot_ = nullptr;

    template<typename U>
    friend class Handle;

    template<typename U>
    friend class MutableHandle;
};

template<typename T>
class MutableHandle final : public PointerOps<T, MutableHandle<T>> {
public:
    static MutableHandle from_slot(Value* slot) { return MutableHandle(slot); }

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>>* = nullptr>
    /* implicit */ MutableHandle(Handle<U> other)
        : slot_(other.slot_) {}

    T get() const { return slot_->as<T>(); }
    void set(T value) { *slot_ = value; }
    /* implicit */ operator T() const { return get(); }
    /* implicit */ operator Value() { return *slot_; }
    /* implicit */ operator Handle<T>() { return Handle<T>::from_slot(slot_); }

private:
    explicit MutableHandle(Value* slot)
        : slot_(slot) {
        HAMMER_ASSERT_NOT_NULL(slot);
    }

    template<typename U>
    friend class MutableHandle;

private:
    Value* slot_;
};

template<typename T>
Handle<T> Root<T>::handle() {
    return Handle<T>::from_slot(&slot_);
}

template<typename T>
MutableHandle<T> Root<T>::mut_handle() {
    return MutableHandle<T>::from_slot(&slot_);
}

} // namespace hammer::vm

#endif // HAMMER_VM_HANDLES_HPP
