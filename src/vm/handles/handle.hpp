#ifndef TIRO_VM_HANDLES_HANDLE_HPP
#define TIRO_VM_HANDLES_HANDLE_HPP

#include "common/span.hpp"
#include "vm/handles/fwd.hpp"
#include "vm/handles/traits.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

// The struct can be declared as a friend to make slot access possible to the underlying APIs.
struct SlotAccess final {
    template<typename T>
    static auto get_slot(const T& instance) {
        return instance.get_slot();
    }

    template<typename T>
    static auto get_slots(const T& instance) {
        return instance.get_slots();
    }
};

// Returns a `Value*` or a `const Value*`, which can be null.
template<typename T>
auto get_slot(const T& instance) {
    return SlotAccess::template get_slot(instance);
}

// Returns a `Span<Value>` or a `Span<const Value>`.
template<typename T>
auto get_slots(const T& instance) {
    return SlotAccess::template get_slots(instance);
}

// Returns a `Value*` or a `const Value*`, which must not be null.
template<typename T>
auto get_valid_slot(const T& instance) {
    auto slot = get_slot(instance);
    TIRO_DEBUG_ASSERT(slot != nullptr, "Invalid slot access.");
    return slot;
}

namespace detail {

// Marker type to enable automatic deduction from the right hand side type.
struct DeduceValueType {};

template<typename Left, typename Right>
using DeducedType =
    std::conditional_t<std::is_same_v<Left, DeduceValueType>, WrappedType<Right>, Left>;

template<typename From, typename To>
using HandleConvertible = std::enable_if_t<
    std::is_same_v<remove_cvref_t<From>, remove_cvref_t<To>> || std::is_convertible_v<From, To>>;

// Support class for operator->() syntax for non-pointer values.
template<typename T>
struct ValueHolder final {
    T value;

    T* operator->() { return std::addressof(value); }
};

inline constexpr Value null_fallback_slot = Value::null();

inline const Value* null_fallback() {
    TIRO_DEBUG_ASSERT(null_fallback_slot.is<Null>(),
        "Null fallback value was corrupted, it must never be written to.");
    return &null_fallback_slot;
}

// For usage with type traits (common base of all simple handles).
struct HandleOpsMarker {};

// Base class for common pointer like handle operations.
// The derived type must implement a single `get_slot()` function that returns a
// slot address (i.e. a `Value*`).
template<typename T, typename Derived>
class HandleOps : public HandleOpsMarker {
public:
    using WrappedType = T;

    /// Deferences the handle's slot and returns the current value.
    ValueHolder<T> operator->() const { return {get()}; }

    /// Deferences the handle's slot and returns the current value.
    T operator*() const { return get(); }

    /// Deferences the handle's slot and returns the current value.
    T get() const { return T(*get_valid_slot(derived())); }

    /// Casts the handle to the requested target type.
    /// Fails with an assertion error if the cast fails.
    template<typename To>
    inline Handle<To> must_cast() const;

    /// Attempts to cast the handle to the requested target type.
    /// Returns an invalid handle if the cast fails.
    template<typename To>
    inline MaybeHandle<To> try_cast() const;

    /// Explicitly convertible to the inner value.
    template<typename To = T, HandleConvertible<T, To>* = nullptr>
    explicit operator To() const {
        return static_cast<To>(get());
    }

private:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

// Provides write only access to a slot.
template<typename T, typename Derived>
class OutHandleOps {
public:
    /// Replaces the handle's current value with the given value.
    template<typename From>
    void set(From&& value) const {
        using Plain = WrappedType<From>;
        static_assert(
            std::is_convertible_v<Plain, T>, "The value must be assignable to the handle's type.");
        *get_valid_slot(derived()) = static_cast<T>(unwrap_value(value));
    }

private:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

// Adds a mutable interface on top of the HandleOps interface.
template<typename T, typename Derived>
class MutHandleOps : public HandleOps<T, Derived>, public OutHandleOps<T, Derived> {
public:
    /// Casts the handle to the requested target type.
    /// Fails with an assertion error if the cast fails.
    template<typename To>
    inline MutHandle<To> must_cast() const;

    /// Attempts to cast the handle to the requested target type.
    /// Returns an invalid handle if the cast fails.
    template<typename To>
    inline MaybeMutHandle<To> try_cast() const;

private:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

template<typename T, template<typename To> typename MaybeHandleTemplate, typename Derived>
class MaybeHandleOps {
public:
    /// Returns true if this handle refers to a valid slot.
    explicit operator bool() const { return valid(); }

    /// Returns true if this handle refers to a valid slot.
    bool valid() const { return get_slot(derived()) != nullptr; }

    /// Attempts to cast this maybe handle to the target type.
    /// Returns an empty maybe handle if this handle is empty or if the current
    /// value does not match the target type.
    template<typename To>
    MaybeHandleTemplate<To> try_cast() const {
        if (auto slot = get_slot(derived()); slot && slot->template is<To>()) {
            return MaybeHandleTemplate<To>::from_raw_slot(slot);
        }
        return MaybeHandleTemplate<To>();
    }

private:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

// Enables implicit conversion from Derived to HandleTemplate<To> if T is
// implicitly convertible to To.
template<typename T, template<typename To> typename HandleTemplate, typename Derived>
class EnableUpcast {
public:
    template<typename To, HandleConvertible<T, To>* = nullptr>
    operator HandleTemplate<To>() const {
        return HandleTemplate<To>::from_raw_slot(get_valid_slot(derived()));
    }

private:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

// Enables implicit conversion from Derived to HandleTemplate<To> if To is
// implicitly convertible to T.
template<typename T, template<typename To> typename HandleTemplate, typename Derived>
class EnableDowncast {
public:
    template<typename To, HandleConvertible<To, T>* = nullptr>
    operator HandleTemplate<To>() const {
        return HandleTemplate<To>::from_raw_slot(get_valid_slot(derived()));
    }

private:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

// Enables implicit conversion from Derived to MaybeHandleTemplate<To> if T is
// implicitly convertible to To.
template<typename T, template<typename To> typename MaybeHandleTemplate, typename Derived>
class EnableMaybeUpcast {
public:
    template<typename To, HandleConvertible<T, To>* = nullptr>
    operator MaybeHandleTemplate<To>() const {
        if (auto slot = get_slot(derived())) {
            return MaybeHandleTemplate<To>::from_raw_slot(slot);
        }
        return MaybeHandleTemplate<To>();
    }

private:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

// Enables implicit conversion from Derived to MaybeHandleTemplate<To> if To is
// implicitly convertible to T.
template<typename T, template<typename To> typename MaybeHandleTemplate, typename Derived>
class EnableMaybeDowncast {
public:
    template<typename To, HandleConvertible<To, T>* = nullptr>
    operator MaybeHandleTemplate<To>() const {
        if (auto slot = get_slot(derived())) {
            return MaybeHandleTemplate<To>::from_raw_slot(slot);
        }
        return MaybeHandleTemplate<To>();
    }

private:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

template<typename T, typename Derived>
template<typename To>
Handle<To> HandleOps<T, Derived>::must_cast() const {
    auto slot = get_valid_slot(derived());
    TIRO_DEBUG_ASSERT(slot->template is<To>(), "Handle: cast to targe type failed.");
    return Handle<To>::from_raw_slot(slot);
}

template<typename T, typename Derived>
template<typename To>
MaybeHandle<To> HandleOps<T, Derived>::try_cast() const {
    auto slot = get_valid_slot(derived());
    return slot->template is<To>() ? MaybeHandle<To>::from_raw_slot(slot) : MaybeHandle<To>();
}

template<typename T, typename Derived>
template<typename To>
MutHandle<To> MutHandleOps<T, Derived>::must_cast() const {
    static_assert(std::is_convertible_v<To, T>, "MutHandle can only cast to child types.");
    auto slot = get_valid_slot(derived());
    TIRO_DEBUG_ASSERT(slot->template is<To>(), "Handle: cast to targe type failed.");
    return MutHandle<To>::from_raw_slot(slot);
}

template<typename T, typename Derived>
template<typename To>
MaybeMutHandle<To> MutHandleOps<T, Derived>::try_cast() const {
    static_assert(std::is_convertible_v<To, T>, "MutHandle can only cast to child types.");
    auto slot = get_valid_slot(derived());
    return slot->template is<To>() ? MaybeMutHandle<To>::from_raw_slot(slot) : MaybeMutHandle<To>();
}

} // namespace detail

template<typename T>
struct WrapperTraits<T, std::enable_if_t<std::is_base_of_v<detail::HandleOpsMarker, T>>> {
    using WrappedType = typename T::WrappedType;
    static WrappedType get(const T& handle) { return handle.get(); }
};

namespace detail {

// A handle base class that is guaranteed to refer to a valid slot.
template<typename T, typename Derived>
class HandleBase {
    static constexpr bool is_const = std::is_const_v<T>;

public:
    using SlotType = std::conditional_t<is_const, const Value, Value>;

    /// Constructs a Handle from a valid slot. The slot must refer to a valid
    /// storage location and it must have already been type-checked.
    static Derived from_raw_slot(SlotType* slot) { return Derived(slot, InternalTag()); }

    /// Constructs a handle that refers to the given slot (which must be a valid pointer).
    explicit HandleBase(T* slot)
        : HandleBase(slot, InternalTag()) {}

private:
    struct InternalTag {};

public:
    explicit HandleBase(SlotType* slot, InternalTag)
        : slot_(slot) {
        TIRO_DEBUG_ASSERT(slot, "The slot must be valid.");
    }

private:
    friend SlotAccess;

    SlotType* get_slot() const { return slot_; }

private:
    SlotType* slot_;
};

// A handle base class that is not guaranteed to refer to a valid slot.
// Access is only allowed after a successful check (via operator bool).
template<typename T, typename Derived>
class MaybeHandleBase {
    static constexpr bool is_const = std::is_const_v<T>;

public:
    using SlotType = std::conditional_t<is_const, const Value, Value>;

    /// Constructs a MaybeHandle from a valid slot. The slot must refer to a valid
    /// storage location and it must have already been type-checked.
    static Derived from_raw_slot(SlotType* slot) { return Derived(slot, InternalTag()); }

    /// Constructs an instance that does not refer to a valid slot.
    MaybeHandleBase() = default;

    /// Constructs an instance that refers to the given slot (which must be a valid pointer).
    explicit MaybeHandleBase(T* slot)
        : MaybeHandleBase(slot, InternalTag()) {}

    MaybeHandleBase(const MaybeHandleBase&) = default;

private:
    struct InternalTag {};

public:
    explicit MaybeHandleBase(SlotType* slot, InternalTag)
        : slot_(slot) {
        TIRO_DEBUG_ASSERT(slot, "The slot must be valid.");
    }

private:
    friend SlotAccess;

    SlotType* get_slot() const { return slot_; }

private:
    SlotType* slot_ = nullptr;
};

} // namespace detail

/// Allows read access to a typed slot.
/// Always refers to a valid, rooted storage location.
/// `Handle<T>` supports the usual handle operations and is implicitly convertible to Handle<Parent>.
template<typename T>
class Handle final : public detail::HandleBase<const T, Handle<T>>,
                     public detail::HandleOps<T, Handle<T>>,
                     public detail::EnableUpcast<T, Handle, Handle<T>> {
public:
    using detail::HandleBase<const T, Handle>::HandleBase;
};

/// Allows read and write access to a typed slot.
/// Always refers to a valid, rooted storage location.
/// Note that mutable handles are not convertible to `MutHandle<Parent>` because that
/// would circumvent type safety.
template<typename T>
class MutHandle final : public detail::HandleBase<T, MutHandle<T>>,
                        public detail::MutHandleOps<T, MutHandle<T>>,
                        public detail::EnableUpcast<T, Handle, MutHandle<T>>,
                        public detail::EnableDowncast<T, OutHandle, MutHandle<T>> {
public:
    using detail::HandleBase<T, MutHandle>::HandleBase;
};

/// Allows write only access to a typed slot.
/// Always refers to a rooted storage location.
/// Out handles are implicitly convertible to derived types.
template<typename T>
class OutHandle final : public detail::HandleBase<T, OutHandle<T>>,
                        public detail::OutHandleOps<T, OutHandle<T>>,
                        public detail::EnableDowncast<T, OutHandle, OutHandle<T>> {
public:
    using detail::HandleBase<T, OutHandle>::HandleBase;
};

/// Allows read access to a typed slot (if present).
/// Accesses to an instance of this type must be preceeded
/// by a check whether this instance is valid.
template<typename T>
class MaybeHandle final : public detail::MaybeHandleBase<const T, MaybeHandle<T>>,
                          public detail::MaybeHandleOps<T, MaybeHandle, MaybeHandle<T>>,
                          public detail::EnableMaybeUpcast<T, MaybeHandle, MaybeHandle<T>> {
public:
    using detail::MaybeHandleBase<const T, MaybeHandle<T>>::MaybeHandleBase;

    template<typename From, std::enable_if_t<std::is_convertible_v<From, Handle<T>>>* = nullptr>
    MaybeHandle(const From& from)
        : MaybeHandle(MaybeHandle::from_raw_slot(get_valid_slot(Handle<T>(from)))) {}

    /// Returns the referenced slot if one is present, or a handle to a statically allocated
    /// null instance otherwise.
    Handle<Nullable<T>> to_nullable() const {
        if (this->valid())
            return handle();
        return Handle<Nullable<T>>::from_raw_slot(detail::null_fallback());
    }

    /// Returns the referenced slot as a handle for read access.
    /// \pre `valid()`.
    Handle<T> handle() const {
        TIRO_DEBUG_ASSERT(this->valid(), "MaybeHandle: accessing invalid handle.");
        return Handle<T>::from_raw_slot(get_valid_slot(*this));
    }
};

/// Allows read and write access to a typed slot (if present).
///
/// Accesses to an instance of this type must be preceeded
/// by a check whether this instance is valid.
///
/// Note that mutable handles are not convertible to `MaybeMutHandle<Parent>` because that
/// would circumvent type safety.
template<typename T>
class MaybeMutHandle final : public detail::MaybeHandleBase<T, MaybeMutHandle<T>>,
                             public detail::MaybeHandleOps<T, MaybeMutHandle, MaybeMutHandle<T>>,
                             public detail::EnableMaybeUpcast<T, MaybeHandle, MaybeMutHandle<T>>,
                             public detail::EnableMaybeDowncast<T, OutHandle, MaybeMutHandle<T>> {
public:
    using detail::MaybeHandleBase<T, MaybeMutHandle<T>>::MaybeHandleBase;

    template<typename From, std::enable_if_t<std::is_convertible_v<From, MutHandle<T>>>* = nullptr>
    MaybeMutHandle(const From& from)
        : MaybeMutHandle(MaybeMutHandle::from_raw_slot(get_valid_slot(MutHandle<T>(from)))) {}

    /// Returns the referenced slot as a handle for read and write access.
    /// \pre `valid()`.
    MutHandle<T> handle() const {
        TIRO_DEBUG_ASSERT(this->valid(), "MaybeHandle: accessing invalid handle.");
        return MutHandle<T>::from_raw_slot(get_valid_slot(*this));
    }
};

/// Allows write access to a typed slot (if present).
///
/// Accesses to an instance of this type must be preceeded
/// by a check whether this instance is valid.
template<typename T>
class MaybeOutHandle final
    : public detail::MaybeHandleBase<T, MaybeOutHandle<T>>,
      public detail::MaybeHandleOps<T, MaybeOutHandle, MaybeOutHandle<T>>,
      public detail::EnableMaybeDowncast<T, MaybeOutHandle, MaybeOutHandle<T>> {
public:
    using detail::MaybeHandleBase<T, MaybeOutHandle<T>>::MaybeHandleBase;

    template<typename From, std::enable_if_t<std::is_convertible_v<From, OutHandle<T>>>* = nullptr>
    MaybeOutHandle(const From& from)
        : MaybeOutHandle(MaybeOutHandle::from_raw_slot(get_valid_slot(OutHandle<T>(from)))) {}

    /// Returns the referenced slot as a handle for read and write access.
    /// \pre `valid()`.
    OutHandle<T> handle() const {
        TIRO_DEBUG_ASSERT(this->valid(), "MaybeHandle: accessing invalid handle.");
        return OutHandle<T>::from_raw_slot(get_valid_slot(*this));
    }
};

/// Converts a Handle<Nullable<T>> to a MaybeHandle<T> that is empty if the value was null.
template<typename HandleLike,
    typename std::enable_if_t<
        is_nullable<WrappedType<
            HandleLike>> && std::is_convertible_v<HandleLike, Handle<WrappedType<HandleLike>>>>* =
        nullptr>
MaybeHandle<typename WrappedType<HandleLike>::ValueType> maybe_null(const HandleLike& h) {
    using T = typename WrappedType<HandleLike>::ValueType;
    auto handle = static_cast<Handle<Nullable<T>>>(h);
    return handle->is_null() ? MaybeHandle<T>() : MaybeHandle<T>(handle.template must_cast<T>());
}

/// Returns a handle that points to a null value.
/// The null value is allocated in static storage and must not be modified.
inline Handle<Null> null_handle() {
    return Handle<Null>::from_raw_slot(detail::null_fallback());
}

} // namespace tiro::vm

#endif // TIRO_VM_HANDLES_HANDLE_HPP
