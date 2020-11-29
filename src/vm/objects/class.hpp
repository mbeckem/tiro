#ifndef TIRO_VM_OBJECTS_CLASS_HPP
#define TIRO_VM_OBJECTS_CLASS_HPP

#include "vm/handles/handle.hpp"
#include "vm/object_support/fwd.hpp"
#include "vm/object_support/layout.hpp"
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
/// to a public `Type` instance, which is exposed to the calling code.
///
/// Multiple InternalType instances may share a common
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

/// Represents public type information (i.e. exposed to bytecode).
// TODO: Slot map like it is planned for objects? i.e. flat array of slots, lookuptable symbol -> index
class Type final : public HeapValue {
private:
    enum Slots {
        NameSlot,
        MembersSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    /// Constructs a new instance that represents a public type.
    /// The key set of the members table must not be modified after construction.
    /// It is however possible to alter the value of an entry (e.g. to implement static mutable fields).
    /// It should always be safe to cache a method returned by a class.
    /// Members are looked up using symbol keys.
    static Type make(Context& ctx, Handle<String> name, Handle<HashTable> members);

    explicit Type(Value v)
        : HeapValue(v, DebugCheck<Type>()) {}

    /// Returns the simple name of the class. This is the name the class
    /// was originally declared with.
    String name();

    /// Attempts to find the member with the given name.
    /// Returns an empty optional on failure.
    std::optional<Value> find_member(Handle<Symbol> name);

    Layout* layout() const { return access_heap<Layout>(); }
};

extern const TypeDesc type_type_desc;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_CLASS_HPP
