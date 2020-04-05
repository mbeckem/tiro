#ifndef TIRO_HEAP_HANDLES_HPP
#define TIRO_HEAP_HANDLES_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/type_traits.hpp"
#include "tiro/objects/value.hpp"
#include "tiro/vm/fwd.hpp"

#include <utility>

namespace tiro::vm {

class RootBase {
public:
    /// The raw address to the slot. Useful for debugging the tracing code.

    uintptr_t slot_address() const noexcept {
        return reinterpret_cast<uintptr_t>(&slot_);
    }

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

class GlobalBase {
public:
    /// The raw address to the slot. Useful for debugging the tracing code.
    uintptr_t slot_address() const {
        return reinterpret_cast<uintptr_t>(&slot_);
    }

protected:
    GlobalBase(Context& ctx, Value value);
    ~GlobalBase();

    GlobalBase(const GlobalBase&) = delete;
    GlobalBase& operator=(const GlobalBase&) = delete;

protected:
    Context& ctx_;
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
                TIRO_DEBUG_NOT_NULL(value);
                return value;
            } else {
                return &value;
            }
        }
    };

    Holder operator->() const { return {self()->get()}; }
    T operator*() const { return self()->get(); }

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

    T get() const { return slot_.as<T>(); }
    void set(T value) { slot_ = value; }
    /* implicit */ operator T() { return get(); }

    inline Handle<T> handle();
    inline MutableHandle<T> mut_handle();

    /* implicit */ operator Handle<T>() { return handle(); }
};

template<typename T>
class Global final : public GlobalBase, public PointerOps<T, Global<T>> {
public:
public:
    Global(Context& ctx)
        : Global(ctx, T()) {}

    Global(Context& ctx, T initial_value)
        : GlobalBase(ctx, initial_value) {}

    Context& ctx() const { return ctx_; }

    T get() const { return slot_.as<T>(); }
    void set(T value) { slot_ = value; }
    /* implicit */ operator T() { return get(); }

    inline Handle<T> handle();
    inline MutableHandle<T> mut_handle();

    /* implicit */ operator Handle<T>() { return handle(); }
};

class HandleBase {
protected:
    inline static const Value null_value_ = Value::null();
};

/// A handle refers to an object that is rooted somewhere else,
/// and is thus guaranteed to survive a garbage collection cycle.
/// Handles should be used as function input arguments.
///
/// A handle must not be used when it is not rooted anymore (e.g. because
/// the original Rooted object was destroyed).
///
/// TODO: Get rid of the hole in the handle "type" system, i.e. "OptionalHandle"
/// for nullable values of type T.
template<typename T>
class Handle final : public HandleBase, public PointerOps<T, Handle<T>> {
public:
    static Handle from_slot(Value* slot) {
        // FIXME type assertion
        return Handle(slot);
    }

    template<typename U,
        std::enable_if_t<std::is_convertible_v<U, T>>* = nullptr>
    /* implicit */ Handle(Handle<U> other)
        : slot_(other.slot_) {
        // Triggers an error if invalid type.
        TIRO_DEBUG_ASSERT((slot_->as<T>(), true), "");
    }

    /* implicit */ Handle()
        : slot_(&null_value_) {}

    // FIXME as_strict probably broken, implement new handles...
    // Not all values are nullable (SmallInteger)
    T get() const { return slot_->as<T>(); }
    /* implicit */ operator T() const { return get(); }
    /* implicit */ operator Value() { return *slot_; }

    // TODO Cleanup
    template<typename U>
    Handle<U> strict_cast() const {
        TIRO_DEBUG_ASSERT(slot_->is<U>(), "Invalid type cast.");
        return Handle<U>(slot_);
    }

    template<typename U>
    Handle<U> cast() const {
        TIRO_DEBUG_ASSERT(
            slot_->is_null() || slot_->is<U>(), "Invalid type cast.");
        return Handle<U>(slot_);
    }

private:
    explicit Handle(const Value* slot)
        : slot_(slot) {
        TIRO_DEBUG_NOT_NULL(slot);
    }

private:
    const Value* slot_;

    template<typename U>
    friend class Handle;

    template<typename U>
    friend class MutableHandle;
};

template<typename T>
class MutableHandle final : public PointerOps<T, MutableHandle<T>> {
public:
    static MutableHandle from_slot(Value* slot) { return MutableHandle(slot); }

    T get() const { return slot_->as<T>(); }
    void set(T value) { *slot_ = value; }
    /* implicit */ operator T() const { return get(); }
    /* implicit */ operator Value() { return *slot_; }
    /* implicit */ operator Handle<T>() { return Handle<T>::from_slot(slot_); }

    /* implicit */
    template<typename U>
    operator Handle<U>() {
        return Handle<U>(Handle<T>::from_slot(slot_));
    }

    // TODO Cleanup
    template<typename U>
    MutableHandle<U> strict_cast() const {
        TIRO_DEBUG_ASSERT(slot_->is<U>(), "Invalid type cast.");
        return MutableHandle<U>(slot_);
    }

    template<typename U>
    MutableHandle<U> cast() const {
        TIRO_DEBUG_ASSERT(
            slot_->is_null() || slot_->is<U>(), "Invalid type cast.");
        return MutableHandle<U>(slot_);
    }

private:
    explicit MutableHandle(Value* slot)
        : slot_(slot) {
        TIRO_DEBUG_NOT_NULL(slot);
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

template<typename T>
Handle<T> Global<T>::handle() {
    return Handle<T>::from_slot(&slot_);
}

template<typename T>
MutableHandle<T> Global<T>::mut_handle() {
    return MutableHandle<T>::from_slot(&slot_);
}

} // namespace tiro::vm

#endif // TIRO_HEAP_HANDLES_HPP
