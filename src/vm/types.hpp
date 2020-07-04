#ifndef TIRO_VM_TYPES_HPP
#define TIRO_VM_TYPES_HPP

#include "common/span.hpp"
#include "vm/fwd.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/hash_table.hpp"

namespace tiro::vm {

// TODO classes and stuff
class TypeSystem {
public:
    // Called by context during construction (Initial phase for setup of internal types):
    void init_internal(Context& ctx);

    // Called by context during construction (Final phase when bootstrapping is complete).
    void init_public(Context& ctx);

    template<typename W>
    inline void walk(W&& w);

    /// Returns a value that represents the type of the given object.
    Value type_of(Handle<Value> object);

    /// Attempts to retrieve the value at the given index from the given object.
    /// Throws an error if the index was invalid (e.g. out of bounds).
    ///
    /// TODO Exceptions!
    Value load_index(Context& ctx, Handle<Value> object, Handle<Value> index);

    /// Attempts to set the value at the given index on the given object.
    /// Throws an error if the index was invalid (e.g. out of bounds).
    ///
    /// TODO Exceptions!
    void store_index(Context& ctx, Handle<Value> object, Handle<Value> index, Handle<Value> value);

    /// Attempts to retrieve the given member property from the given object.
    /// Returns no value if there is no such member.
    std::optional<Value> load_member(Context& ctx, Handle<Value> object, Handle<Symbol> member);

    /// Attempts to store the given property value. Returns false if the property
    /// could not be written (does not exist, or is read only).
    ///
    /// TODO Exceptions!
    bool
    store_member(Context& ctx, Handle<Value> object, Handle<Symbol> member, Handle<Value> value);

    /// This function is called for the `object.member(...)` method call syntax.
    /// Returns a member function suitable for invocation on the given instance.
    /// Note that, depending on the function returned here, the call must
    /// be made in different ways (native functions, this pointer, etc.).
    ///
    /// The function value returned here does not need to be a real method - it may be a simple function
    /// that is accessible as the property `object.member`.
    std::optional<Value> load_method(Context& ctx, Handle<Value> object, Handle<Symbol> member);

    /// Returns the builtin type object for the given value type, suitable for object construction.
    /// The returned value is always rooted and does not change after initialization.
    /// Special care has to be taken with types during bootstrap, see init().
    template<typename T>
    Header* internal_type() {
        auto type = internal_types_[type_index<T>()];
        TIRO_DEBUG_ASSERT(!type.is_null(),
            "The requested type has not been initialized correctly. "
            "This may be an ordering error during the type initialization phase.");
        return type.heap_ptr();
    }

private:
    static constexpr size_t total_internal_types = static_cast<size_t>(max_value_type) + 1;

    template<typename T>
    static constexpr size_t type_index() {
        return type_index(TypeToTag<T>);
    }

    static constexpr size_t type_index(ValueType builtin_type) {
        size_t index = static_cast<size_t>(builtin_type);
        TIRO_DEBUG_ASSERT(index < total_internal_types, "Builtin type index out of bounds.");
        return index;
    }

private:
    // TODO: Remove (superseded by builtin_types_).
    std::array<Type, total_internal_types> public_types_{};
    std::array<InternalType, total_internal_types> internal_types_{};
};

template<typename W>
void TypeSystem::walk(W&& w) {
    for (auto& type : public_types_) {
        w(type);
    }

    for (auto& type : internal_types_) {
        w(type);
    }
}

} // namespace tiro::vm

#endif // TIRO_VM_TYPES_HPP
