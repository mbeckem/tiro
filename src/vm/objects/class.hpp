#ifndef TIRO_VM_OBJECTS_CLASS_HPP
#define TIRO_VM_OBJECTS_CLASS_HPP

#include "vm/handles/handle.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/type_desc.hpp"
#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

/// A method is part of a class and contains a function
/// that can be called with a class instance as the first argument.
// TODO Point to the containing class
// TODO Needed at all?
class Method final : public HeapValue {
private:
    enum Slots {
        FunctionSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    static Method make(Context& ctx, Handle<Value> function);

    explicit Method(Value v)
        : HeapValue(v, DebugCheck<Method>()) {}

    Value function();

    Layout* layout() const { return access_heap<Layout>(); }
};

/// A InternalType instance represents type information for builtin types.
/// Instances of this type are not exposed to the public. Instead, they point
/// to a `Type` instance. Multiple InternalType instances may share a common
/// public Type (e.g. all different flavours of functions have the same public type).
class InternalType final : public HeapValue {
private:
    enum Slots {
        PublicTypeSlot,
        SlotCount_,
    };

    struct Payload {
        ValueType builtin_type;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    /// Constructs the root type. The root type is its own type.
    static InternalType make_root(Context& ctx);

    /// Constructs a new class object for the given builtin type.
    /// This function requires the root type to be initialized and available through the context.
    static InternalType make(Context& ctx, ValueType builtin_type);

    explicit InternalType(Value v)
        : HeapValue(v, DebugCheck<InternalType>()) {}

    /// Returns the kind of builtin object instances represented by this type instance.
    ValueType builtin_type();

    /// The public type represents this type to calling code.
    Nullable<Type> public_type();
    void public_type(MaybeHandle<Type> type);

    Layout* layout() const { return access_heap<Layout>(); }

private:
    friend HeapValue;

    struct Unchecked {};

    // Called only when it is statically known that "header" represents a InternalType instance.
    // This constructor exists to avoid infinite recursion during debug mode typ checking: a
    // type instance would be created for the checked object, which would need to be verified (is it actually a type?)
    // which would recurse and run into a stack overflow because the root type is its own type.
    explicit InternalType(Header* header, Unchecked)
        : HeapValue(header) {}
};

class Type final : public HeapValue {
private:
    enum Slots {
        NameSlot,
        MethodsSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    /// Constructs a new instance that represents a public type.
    /// The method table object must not be modified after construction is complete.
    /// It should always be safe to cache a method returned by a class.
    /// Methods are looked up using symbol keys.
    static Type make(Context& ctx, Handle<String> name, Handle<HashTable> methods);

    explicit Type(Value v)
        : HeapValue(v, DebugCheck<Type>()) {}

    /// Returns the simple name of the class. This is the name the class
    /// was originally declared with.
    String name();

    /// Attempts to find the method with the given name.
    /// Returns an empty optional on failure.
    std::optional<Method> find_method(Handle<Symbol> name);

    Layout* layout() const { return access_heap<Layout>(); }
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

extern const TypeDesc type_type_desc;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_CLASS_HPP
