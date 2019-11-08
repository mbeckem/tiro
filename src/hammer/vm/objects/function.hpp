#ifndef HAMMER_VM_OBJECTS_FUNCTION_HPP
#define HAMMER_VM_OBJECTS_FUNCTION_HPP

#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/value.hpp"

#include "hammer/core/span.hpp"

namespace hammer::vm {

/**
 * Represents executable byte code, typically used to 
 * represents the instructions within a function.
 * 
 * TODO: Need a bytecode validation routine.
 */
class Code final : public Value {
public:
    static Code make(Context& ctx, Span<const byte> code);

    Code() = default;

    explicit Code(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Code>(), "Value is not a code object.");
    }

    const byte* data() const noexcept;

    size_t size() const noexcept;

    Span<const byte> view() const noexcept { return {data(), size()}; }

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/**
 * Represents a function prototype.
 * 
 * Function prototypes contain the static properties of functions and are referenced
 * by the actual function instances. Function prototypes are a necessary implementation
 * detail because actual functions (i.e. with closures) share all static properties
 * but have different closure variables each.
 * 
 * Function prototypes can be thought of as the 'class' of a function.
 */
class FunctionTemplate final : public Value {
public:
    static FunctionTemplate make(Context& ctx, Handle<String> name,
        Handle<Module> module, u32 params, u32 locals, Span<const byte> code);

    FunctionTemplate() = default;

    explicit FunctionTemplate(Value v)
        : Value(v) {
        HAMMER_ASSERT(
            v.is<FunctionTemplate>(), "Value is not a function template.");
    }

    String name() const noexcept;
    Module module() const noexcept;
    Code code() const noexcept;
    u32 params() const noexcept;
    u32 locals() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

/**
 * Represents captured variables from an upper scope captured by a nested function.
 * ClosureContexts point to their parent (or null if they are at the root).
 */
class ClosureContext final : public Value {
public:
    static ClosureContext
    make(Context& ctx, size_t size, Handle<ClosureContext> parent);

    ClosureContext() = default;

    explicit ClosureContext(Value v)
        : Value(v) {
        HAMMER_ASSERT(
            v.is<ClosureContext>(), "Value is not a closure context.");
    }

    ClosureContext parent() const noexcept;

    const Value* data() const noexcept;
    size_t size() const noexcept;
    Span<const Value> values() const { return {data(), size()}; }

    Value get(size_t index) const;
    void set(WriteBarrier, size_t index, Value value) const;

    // level == 0 -> return *this. Returns null in the unlikely case that the level is invalid.
    ClosureContext parent(size_t level) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

/**
 * Represents a function value.
 * 
 * A function can be thought of a pair of a closure context and a function template:
 * 
 *  - The function template contains the static properties (parameter declarations, bytecode, ...)
 *    and is never null. All closure function that are constructed by the same function declaration
 *    share a common function template instance.
 *  - The closure context contains the captured variables bound to this function object
 *    and can be null.
 *  - The function combines the two.
 * 
 * Only the function type is exposed within the language.
 */
class Function final : public Value {
public:
    static Function make(Context& ctx, Handle<FunctionTemplate> tmpl,
        Handle<ClosureContext> closure);

    Function() = default;

    explicit Function(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Function>(), "Value is not a function.");
    }

    FunctionTemplate tmpl() const noexcept;
    ClosureContext closure() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_FUNCTION_HPP
