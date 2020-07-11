#ifndef TIRO_VM_HANDLES_GLOBAL_HPP
#define TIRO_VM_HANDLES_GLOBAL_HPP

#include "vm/handles/handle.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

namespace detail {

class GlobalBase {
public:
    Context& ctx() const { return ctx_; }

protected:
    GlobalBase(Context& ctx, Value initial);
    ~GlobalBase();

    GlobalBase(const GlobalBase&) = delete;
    GlobalBase& operator=(const GlobalBase&) = delete;

protected:
    friend SlotAccess;

    Value* get_slot() const { return &slot_; }

protected:
    Context& ctx_;
    mutable Value slot_;
};

} // namespace detail

/// A slot that is dynamically registered with the context.
/// These are pretty inefficient compared to locals and should only
/// be used when absolutely necessary.
///
/// Globals register themselves with the context on creation and unregister themselves
/// on destruction.
///
/// Globals are neither copyable nor movable. Allocate them on the heap if necessary.
// TODO: Make this class safe to use when the context is destroyed before the globals?
// The context would have to set a flag in the globals so they can destruct without
// accessing the context again.
template<typename T>
class Global final : public detail::MutHandleOps<T, Global<T>>,
                     public detail::EnableUpcast<T, Handle, Global<T>>,
                     public detail::GlobalBase {
public:
    /// Creates a new global slot with the given initial value.
    /// The new slot is automatically added to the root set of the context.
    explicit Global(Context& ctx, T initial)
        : GlobalBase(ctx, initial) {}

    /// Removes the slot from the context's root set.
    ~Global() = default;

    Global(Global&&) = delete;
    Global& operator=(Global&&) = delete;

    /// Returns a mutable handle that point to the same slot as this global.
    MutHandle<T> mut() { return MutHandle<T>::from_raw_slot(get_slot()); }

    /// Returns an output handle that point to the same slot as this global.
    OutHandle<T> out() { return OutHandle<T>::from_raw_slot(get_slot()); }
};

template<typename T>
Global<WrappedType<T>> global(Context& ctx, T&& initial = T()) {
    return Global<WrappedType<T>>{ctx, unwrap_value(initial)};
}

} // namespace tiro::vm

#endif // TIRO_VM_HANDLES_GLOBAL_HPP
