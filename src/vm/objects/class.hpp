#ifndef TIRO_VM_OBJECTS_CLASS_HPP
#define TIRO_VM_OBJECTS_CLASS_HPP

#include "vm/objects/layout.hpp"
#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

/// A method is part of a class and contains a function
/// that can be called with a class instance as the first argument.
// TODO Point to the containing class
class Method final : public HeapValue {
private:
    enum Slots {
        FunctionSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    static Method make(Context& ctx, Handle<Value> function);

    Method() = default;

    explicit Method(Value v)
        : HeapValue(v, DebugCheck<Method>()) {}

    Value function();

    Layout* layout() const { return access_heap<Layout>(); }
};

/// A Type instance represents type information for builtin types.
/// This includes the class name and a table of methods.
// TODO: User defined classes.
class Type final : public HeapValue {
private:
    enum Slots {
        NameSlot,
        MethodsSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    /// Constructs a new class object with the given name and method table.
    ///
    /// The method table object must not be modified. It should always be safe to
    /// cache a method returned by a class. Methods are looked up using symbol keys.
    static Type make(Context& ctx, Handle<String> name, Handle<HashTable> methods);

    Type() = default;

    explicit Type(Value v)
        : HeapValue(v, DebugCheck<Type>()) {}

    /// Returns the simple name of the class. This is the name the class
    /// was originally declared with.
    String name();

    /// Attempts to find the method with the given name.
    /// Returns an empty optional on failure.
    std::optional<Method> find_method(Handle<Symbol> name);

    Layout* layout() const { return access_heap<Layout>(); }

private:
    HashTable get_methods();
};

/// An object with arbitrary, dynamic properties.
/// Properties are addressed using symbols.
///
/// TODO: This will eventually be removed and replaced by real classes.
class DynamicObject final : public HeapValue {
private:
    enum {
        PropertiesSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    static DynamicObject make(Context& ctx);

    DynamicObject() = default;

    explicit DynamicObject(Value v)
        : HeapValue(v, DebugCheck<DynamicObject>()) {}

    /// Returns an array of property names for this object.
    Array names(Context& ctx);

    /// Returns the property with the given name. Returns null if that property
    /// does not exist.
    Value get(Handle<Symbol> name);

    /// Sets the property to the given value. Setting a property to null removes
    /// that property.
    void set(Context& ctx, Handle<Symbol> name, Handle<Value> value);

    Layout* layout() const { return access_heap<Layout>(); }

private:
    HashTable get_props();
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_CLASS_HPP
