#ifndef TIRO_VM_TYPES_HPP
#define TIRO_VM_TYPES_HPP

#include "common/adt/span.hpp"
#include "vm/fwd.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/exception.hpp"
#include "vm/objects/public_types.hpp"

#include <array>

namespace tiro::vm {

[[nodiscard]] Exception function_call_not_supported_exception(Context& ctx, Handle<Value> value);

[[nodiscard]] Exception
assertion_failed_exception(Context& ctx, Handle<String> expr, MaybeHandle<String> message);

class TypeSystem {
public:
    // Called by context during construction (Initial phase for setup of internal types):
    void init_internal(Context& ctx);

    // Called by context during construction (Final phase when bootstrapping is complete).
    void init_public(Context& ctx);

    /// Returns the type instance that represents the given public type.
    Type type_of(PublicType pt);

    /// Returns a value that represents the type of the given object.
    /// Equivalent to looking up the public type value for the object's actual vm object type
    /// and then returning the corresponding type instance.
    ///
    /// Throws if the builtin type is not exposed to the public.
    Type type_of(Handle<Value> object);

    /// Returns a value that represents the public type of the given builtin object type.
    /// Equivalent to looking up the public type value for the object's actual vm object type
    /// and then returning the corresponding type instance.
    ///
    /// Throws if the builtin type is not exposed to the public.
    Type type_of(ValueType builtin);

    /// Attempts to retrieve the value at the given index from the given object.
    /// Throws an error if the index was invalid (e.g. out of bounds).
    Fallible<Value> load_index(Context& ctx, Handle<Value> object, Handle<Value> index);

    /// Attempts to set the value at the given index on the given object.
    /// Throws an error if the index was invalid (e.g. out of bounds).
    Fallible<void>
    store_index(Context& ctx, Handle<Value> object, Handle<Value> index, Handle<Value> value);

    /// Attempts to retrieve the given member property from the given object.
    /// Returns no value if there is no such member.
    Fallible<Value> load_member(Context& ctx, Handle<Value> object, Handle<Symbol> member);

    /// Attempts to store the given property value. Returns false if the property
    /// could not be written (does not exist, or is read only).
    Fallible<void>
    store_member(Context& ctx, Handle<Value> object, Handle<Symbol> member, Handle<Value> value);

    /// This function is called for the `object.member(...)` method call syntax.
    /// Returns a member function suitable for invocation on the given instance.
    /// Note that, depending on the function returned here, the call must
    /// be made in different ways (native functions, this pointer, etc.).
    ///
    /// The function value returned here does not need to be a real method - it may be a simple function
    /// that is accessible as the property `object.member`.
    Fallible<Value> load_method(Context& ctx, Handle<Value> object, Handle<Symbol> member);

    /// Constructs an iterator for the given object (if supported).
    /// TODO: Implement useful iterator protocol so we dont have to special case stuff in here.
    Fallible<Value> iterator(Context& ctx, Handle<Value> object);

    /// Advances the given iterator to the next element. Returns an empty optional if the iterator is at the end.
    /// TODO: Implement useful iterator protocol so we dont have to special case stuff in here.
    Fallible<std::optional<Value>> iterator_next(Context& ctx, Handle<Value> iterator);

    /// Returns the builtin type object for the given value type, suitable for object construction.
    /// The returned value is always rooted and does not change after initialization.
    /// Special care has to be taken with types during bootstrap, see init().
    template<typename T>
    Header* raw_internal_type() {
        auto type = internal_types_[value_type_index<T>()];
        TIRO_DEBUG_ASSERT(!type.is_null(),
            "The requested type has not been initialized correctly. "
            "This may be an ordering error during the type initialization phase.");
        return type.value().heap_ptr();
    }

    // Walk all object references rooted in this object.
    template<typename Tracer>
    inline void trace(Tracer&& tracer);

private:
    static constexpr size_t total_public_types = static_cast<size_t>(max_public_type) + 1;
    static constexpr size_t total_internal_types = static_cast<size_t>(max_value_type) + 1;

    static constexpr size_t public_type_index(PublicType public_type) {
        size_t index = static_cast<size_t>(public_type);
        TIRO_DEBUG_ASSERT(index < total_public_types, "Public type index out of bounds.");
        return index;
    }

    template<typename T>
    static constexpr size_t value_type_index() {
        return value_type_index(TypeToTag<T>);
    }

    static constexpr size_t value_type_index(ValueType builtin_type) {
        size_t index = static_cast<size_t>(builtin_type);
        TIRO_DEBUG_ASSERT(index < total_internal_types, "Builtin type index out of bounds.");
        return index;
    }

private:
    std::array<Nullable<Type>, total_public_types> public_types_{};
    std::array<Nullable<InternalType>, total_internal_types> internal_types_{};
};

template<typename Tracer>
void TypeSystem::trace(Tracer&& tracer) {
    for (auto& type : public_types_) {
        tracer(type);
    }

    for (auto& type : internal_types_) {
        tracer(type);
    }
}

} // namespace tiro::vm

#endif // TIRO_VM_TYPES_HPP
