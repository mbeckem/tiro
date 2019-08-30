#ifndef HAMMER_VM_OBJECT_HPP
#define HAMMER_VM_OBJECT_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/error.hpp"
#include "hammer/core/math.hpp"
#include "hammer/core/span.hpp"
#include "hammer/core/type_traits.hpp"
#include "hammer/vm/handles.hpp"
#include "hammer/vm/value.hpp"

#include <new>
#include <string_view>

namespace hammer::vm {

class Context;

// Helper structure to force the use of the write barrier macro.
// Only the context can create barrier objects.
class WriteBarrier {
private:
    WriteBarrier() = default;

    friend Context;
};

/**
 * Represents the null value. All null values have the same representation Value::null().
 */
class Null final : public Value {
public:
    static Null make(Context& ctx);

    Null() = default;

    explicit Null(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is_null(), "Value is not null.");
    }

    size_t object_size() const noexcept { return 0; }

    template<typename W>
    void walk(W&&) {}
};

/**
 * Instances of Undefined are used as a sentinel value for uninitialized values.
 * They are never leaked into user code. Accesses that generate an undefined value
 * produce an error instead.
 * 
 * There is only one instance for each context.
 */
class Undefined final : public Value {
public:
    static Undefined make(Context& ctx);

    Undefined() = default;

    explicit Undefined(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Undefined>(), "Value is not undefined.");
    }

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/**
 * Instances represent the boolean "true" or "false.
 * 
 * There is only one instance for each context.
 */
class Boolean final : public Value {
public:
    static Boolean make(Context& ctx, bool value);

    Boolean() = default;

    explicit Boolean(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Boolean>(), "Value is not a boolean.");
    }

    bool value() const noexcept;
    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/**
 * Represents a heap-allocated 64-bit integer value.
 * 
 * TODO: Small integer optimization.
 */
class Integer final : public Value {
public:
    static Integer make(Context& ctx, i64 value);

    Integer() = default;

    explicit Integer(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Integer>(), "Value is not an integer.");
    }

    i64 value() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/**
 * Represents a heap-allocated 64-bit floating point value.
 */
class Float final : public Value {
public:
    static Float make(Context& ctx, double value);

    Float() = default;

    explicit Float(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Float>(), "Value is not a float.");
    }

    double value() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/**
 * Represents a string. 
 * 
 * TODO: Unicode stuff.
 */
class String final : public Value {
public:
    static String make(Context& ctx, std::string_view str);

    String() = default;

    explicit String(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<String>(), "Value is not a string.");
    }

    std::string_view view() const noexcept { return {data(), size()}; }

    const char* data() const noexcept;
    size_t size() const noexcept;

    size_t hash() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/**
 * Represents executable byte code.
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
 */
class FunctionTemplate final : public Value {
public:
    static FunctionTemplate make(Context& ctx, Handle<String> name, Handle<Module> module,
                                 Handle<Array> literals, u32 params, u32 locals,
                                 Span<const byte> code);

    FunctionTemplate() = default;

    explicit FunctionTemplate(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<FunctionTemplate>(), "Value is not a function template.");
    }

    String name() const noexcept;
    Module module() const noexcept;
    Array literals() const noexcept;
    Code code() const noexcept;
    u32 params() const noexcept;
    u32 locals() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

// TODO
// class FunctionClosure final : public Value {};

/**
 * Represents a function value (template plus the optional bound closure environment).
 */
class Function final : public Value {
public:
    static Function make(Context& ctx, Handle<FunctionTemplate> tmpl, Handle<Value> closure);

    Function() = default;

    explicit Function(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Function>(), "Value is not a function.");
    }

    FunctionTemplate tmpl() const noexcept;
    Value closure() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

/**
 * Represents a module, which is a collection of exported and private members.
 */
class Module final : public Value {
public:
    static Module make(Context& ctx, Handle<String> name, Handle<Array> members);

    Module() = default;

    explicit Module(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Module>(), "Value is not a module.");
    }

    String name() const noexcept;
    Array members() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

/**
 * An array is a sequence of values allocated in a contiguous block on the heap.
 */
class Array final : public Value {
public:
    static Array make(Context& ctx, size_t size);

    Array() = default;

    explicit Array(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Array>(), "Value is not an array.");
    }

    const Value* data() const noexcept;
    size_t size() const noexcept;
    Span<const Value> values() const { return {data(), size()}; }

    Value get(size_t index) const noexcept;
    void set(WriteBarrier, size_t index, Value value) const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

/**
 * Serves as a call & value stack for a coroutine. Values pushed/popped by instructions
 * are located here, as well as function call frames. The stack's memory is contiguous.
 * 
 * A new stack that is the copy of an old stack (with the same content but with a larger size)
 * can be obtained via CoroutineStack::grow(). Care must be taken with pointers into the old stack
 * (such as existing frame pointes) as they will be different for the new stack.
 */
class CoroutineStack final : public Value {
public:
    struct alignas(Value) Frame {
        Frame* caller = nullptr;       // Points upwards the stack
        FunctionTemplate tmpl;         // Contains executable code etc.
        Value closure = Value::null(); // Closure (If any)
        u32 args = 0;                  // this many values BEFORE the frame
        u32 locals = 0;                // this many values AFTER the frame
        const byte* pc = nullptr;      // Program counter, points into tmpl->code
    };

    // alignment of Frame could be higher than value, then we would have to pad.
    // It cannot be lower.
    static_assert(alignof(Frame) == alignof(Value), "Required for stack operations.");

    static_assert(std::is_trivially_destructible_v<Value>);
    static_assert(std::is_trivially_destructible_v<Frame>);
    static_assert(std::is_trivially_copyable_v<Value>);
    static_assert(std::is_trivially_copyable_v<Frame>);

public:
    static CoroutineStack make(Context& ctx, u32 stack_size);

    // new_size must be greater than the old stack size.
    static CoroutineStack grow(Context& ctx, Handle<CoroutineStack> old_stack, u32 new_size);

    CoroutineStack() = default;

    explicit CoroutineStack(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<CoroutineStack>(), "Value is not a coroutine stack.");
    }

    /// Pushes a frame for given function template + closure on the stack.
    /// There must be enough arguments already on the stack to satisfy the function template.
    bool push_frame(FunctionTemplate tmpl, Value closure);

    /// Returns the top call frame, or null.
    Frame* top_frame();

    /// Removes the top call frame.
    void pop_frame();

    /// The current call frame's arguments.
    Span<Value> args();

    /// The current call frame's local variables.
    Span<Value> locals();

    /// Push a value on the current frame's value stack.
    bool push_value(Value v);

    /// Returns the number of values on the current frame's value stack.
    u32 top_value_count();

    /// Returns a pointer to the topmost value on the current frame's value stack.
    Value* top_value();

    /// Returns a pointer to the n-th topmost value (0 is the the topmost value) on the current
    /// frame's value stack.
    Value* top_value(u32 n);

    /// Removes the topmost value from the current frame's value stack.
    void pop_value();

    /// Removes the n topmost values from the current frame's value stack.
    void pop_values(u32 n);

    u32 stack_used() const noexcept;
    u32 stack_size() const noexcept;
    u32 stack_available() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    // Begin and end of the frame's call arguments.
    Value* args_begin(Frame* frame);
    Value* args_end(Frame* frame);

    // Begin and end of the frame's local variables.
    Value* locals_begin(Frame* frame);
    Value* locals_end(Frame* frame);

    // Begin and end of the frame's value stack.
    Value* values_begin(Frame* frame);
    Value* values_end(Frame* frame, byte* max);

    // Number of values on the frame's value stack.
    u32 value_count(Frame* frame, byte* max);

private:
    struct Data;

    inline Data* data() const noexcept;
};

enum class CoroutineState { Ready, Running, Done };

/**
 * A coroutine is a lightweight userland thread. Coroutines are multiplexed
 * over actual operating system threads.
 */
class Coroutine final : public Value {
public:
    static Coroutine make(Context& ctx, Handle<String> name, Handle<CoroutineStack> stack);

    Coroutine() = default;

    explicit Coroutine(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Coroutine>(), "Value is not a coroutine.");
    }

    String name() const noexcept;

    CoroutineStack stack() const noexcept;
    void stack(WriteBarrier, Handle<CoroutineStack> stack) noexcept;

    Value result() const noexcept;
    void result(WriteBarrier, Handle<Value> result) noexcept;

    CoroutineState state() const noexcept;
    void state(CoroutineState state) noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

// Will be used to implement write barriers in the future.
#define HAMMER_WRITE_MEMBER(ctx, obj, member, new_value) \
    (obj).member((ctx).write_barrier(), (new_value))

#define HAMMER_WRITE_INDEX(ctx, obj, index, new_value) \
    (obj).set((ctx).write_barrier(), (index), (new_value))

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECT_HPP
