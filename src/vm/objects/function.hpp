#ifndef TIRO_VM_OBJECTS_FUNCTION_HPP
#define TIRO_VM_OBJECTS_FUNCTION_HPP

#include "common/span.hpp"
#include "vm/handles/handle.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

/// Represents executable byte code, typically used to
/// represents the instructions within a function.
///
/// TODO: Need a bytecode validation routine.
/// TODO: Code should not be movable on the heap.
class Code final : public HeapValue {
public:
    using Layout = BufferLayout<byte, alignof(byte)>;

    static Code make(Context& ctx, Span<const byte> code);

    explicit Code(Value v)
        : HeapValue(v, DebugCheck<Code>()) {}

    const byte* data();
    size_t size();
    Span<const byte> view() { return {data(), size()}; }

    Layout* layout() const { return access_heap<Layout>(); }
};

/// Represents a function prototype.
///
/// Function prototypes contain the static properties of functions and are referenced
/// by the actual function instances. Function prototypes are a necessary implementation
/// detail because actual functions (i.e. with closures) share all static properties
/// but have different closure variables each.
class FunctionTemplate final : public HeapValue {
private:
    struct Payload {
        u32 params;
        u32 locals;
    };

    enum Slots {
        NameSlot,
        ModuleSlot,
        CodeSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static FunctionTemplate make(Context& ctx, Handle<String> name, Handle<Module> module,
        u32 params, u32 locals, Span<const byte> code);

    explicit FunctionTemplate(Value v)
        : HeapValue(v, DebugCheck<FunctionTemplate>()) {}

    /// The name of the function.
    String name();

    /// The module the function belongs to.
    Module module();

    /// The executable byte code of this function.
    Code code();

    /// The (minimum) number of required parameters.
    u32 params();

    /// The number of local variables used by the function. These must be allocated
    /// on the stack before the function may execute.
    u32 locals();

    Layout* layout() const { return access_heap<Layout>(); }
};

/// Represents captured variables from an upper scope captured by a nested function.
/// ClosureContexts point to their parent (or null if they are at the root).
class Environment final : public HeapValue {
private:
    enum Slots {
        ParentSlot,
        SlotCount_,
    };

public:
    using Layout = FixedSlotsLayout<Value, StaticSlotsPiece<SlotCount_>>;

    static Environment make(Context& ctx, size_t size, MaybeHandle<Environment> parent);

    explicit Environment(Value v)
        : HeapValue(v, DebugCheck<Environment>()) {}

    Nullable<Environment> parent();

    Value* data();
    size_t size();
    Span<Value> values() { return {data(), size()}; }

    Value get(size_t index);
    void set(size_t index, Value value);

    // level == 0 -> return *this. Returns null in the unlikely case that the level is invalid.
    Nullable<Environment> parent(size_t level);

    Layout* layout() const { return access_heap<Layout>(); }
};

/// Represents a function value.
///
/// A function can be thought of a pair of a closure context and a function template:
///
///  - The function template contains the static properties (parameter declarations, bytecode, ...)
///    and is never null. All closure function that are constructed by the same function declaration
///    share a common function template instance.
///  - The closure context contains the captured variables bound to this function object
///    and can be null.
///  - The function combines the two.
///
/// Only the function type is exposed within the language.
class Function final : public HeapValue {
private:
    enum Slots {
        TmplSlot,
        ClosureSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    static Function
    make(Context& ctx, Handle<FunctionTemplate> tmpl, MaybeHandle<Environment> closure);

    explicit Function(Value v)
        : HeapValue(v, DebugCheck<Function>()) {}

    FunctionTemplate tmpl();
    Nullable<Environment> closure();

    Layout* layout() const { return access_heap<Layout>(); }
};

/// A function where the first parameter ("this") has been bound
/// and will be automatically passed as the first argument
/// of the wrapped function.
class BoundMethod final : public HeapValue {
private:
    enum Slots {
        FunctionSlot,
        ObjectSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    static BoundMethod make(Context& ctx, Handle<Value> function, Handle<Value> object);

    explicit BoundMethod(Value v)
        : HeapValue(v, DebugCheck<BoundMethod>()) {}

    Value function();
    Value object();

    Layout* layout() const { return access_heap<Layout>(); }
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_FUNCTION_HPP
