#ifndef TIRO_VM_TYPES_HPP
#define TIRO_VM_TYPES_HPP

#include "common/span.hpp"
#include "vm/fwd.hpp"
#include "vm/objects/hash_tables.hpp"

#include <unordered_map>

namespace tiro::vm {

// TODO classes and stuff
class TypeSystem {
public:
    // Called by context
    void init(Context& ctx);

    template<typename W>
    inline void walk(W&& w);

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

    /// Returns a member function suitable for invocation on the given instance, i.e.
    /// `object.member(...)` is valid syntax. Note that, depending on the function
    /// returned here, the call must be made in different ways (native functions, this pointer, etc.).
    ///
    /// The function value returned here does not need to be a real method - it may be an already bound function
    /// that is accessible as the property `object.member`.
    std::optional<Value> load_method(Context& ctx, Handle<Value> object, Handle<Symbol> member);

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

} // namespace tiro::vm

#endif // TIRO_VM_TYPES_HPP
