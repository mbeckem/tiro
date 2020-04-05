#ifndef TIRO_OBJECTS_CLASSES_HPP
#define TIRO_OBJECTS_CLASSES_HPP

#include "tiro/objects/value.hpp"

namespace tiro::vm {

/// A method is part of a class and contains a function
/// that can be called with a class instance as the first argument.
class Method final : public Value {
public:
    // TODO Point to the containing class

    static Method make(Context& ctx, Handle<Value> function);

    Method() = default;

    explicit Method(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<Method>(), "Value is not a method.");
    }

    Value function() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    inline Data* access_heap() const;
};

// TODO: Whats the best way to implement symbols? We already have interned strings!
class Symbol final : public Value {
public:
    /// String must be interned.
    static Symbol make(Context& ctx, Handle<String> name);

    Symbol() = default;

    explicit Symbol(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<Symbol>(), "Value is not a symbol.");
    }

    String name() const;

    bool equal(Symbol other) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    inline Data* access_heap() const;
};

/// An object with arbitrary, dynamic properties.
/// Properties are addressed using symbols.
///
/// TODO: This will eventually be removed and replaced by real classes.
class DynamicObject final : public Value {
public:
    static DynamicObject make(Context& ctx);

    DynamicObject() = default;

    explicit DynamicObject(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(
            v.is<DynamicObject>(), "Value is not a dynamic object.");
    }

    // Returns an array of property names for this object.
    Array properties(Context& ctx) const;

    // Returns the property with the given name. Returns null if that property
    // does not exist.
    Value get(Handle<Symbol> property) const;

    // Sets the property to the given value. Setting a property to null removes
    // that property.
    void set(Context& ctx, Handle<Symbol> property, Handle<Value> value);

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    inline Data* access_heap() const;
};

} // namespace tiro::vm

#endif // TIRO_OBJECTS_CLASSES_HPP
