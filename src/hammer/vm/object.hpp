#ifndef HAMMER_VM_OBJECT_HPP
#define HAMMER_VM_OBJECT_HPP

#include "hammer/core/defs.hpp"
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
 * Represents a function value (template plus the optional bound closure environment).
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

/**
 * Represents a module, which is a collection of exported and private members.
 */
class Module final : public Value {
public:
    static Module
    make(Context& ctx, Handle<String> name, Handle<FixedArray> members);

    Module() = default;

    explicit Module(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Module>(), "Value is not a module.");
    }

    String name() const noexcept;
    FixedArray members() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

/**
 * An array is a sequence of values allocated in a contiguous block on the heap.
 */
class FixedArray final : public Value {
public:
    static FixedArray make(Context& ctx, size_t size);

    static FixedArray
    make(Context& ctx, /* FIXME must be rooted */ Span<const Value> values);

    // total_size must be greater than values.size()
    static FixedArray make(Context& ctx,
        /* FIXME must be rooted */ Span<const Value> values, size_t total_size);

    FixedArray() = default;

    explicit FixedArray(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<FixedArray>(), "Value is not a fixed array.");
    }

    const Value* data() const noexcept;
    size_t size() const noexcept;
    Span<const Value> values() const { return {data(), size()}; }

    Value get(size_t index) const;
    void set(WriteBarrier, size_t index, Value value) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

/**
 * A dynamic, resizable array.
 */
class Array final : public Value {
public:
    static Array make(Context& ctx, size_t initial_capacity);

    static Array make(Context& ctx,
        /* FIXME must be rooted */ Span<const Value> initial_content);

    Array() = default;

    explicit Array(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Array>(), "Value is not an array.");
    }

    const Value* data() const noexcept;
    size_t size() const noexcept;
    Span<const Value> values() const noexcept { return {data(), size()}; }

    size_t capacity() const noexcept;

    Value get(size_t index) const;
    void set(Context& ctx, size_t index, Value value) const;

    void append(Context& ctx, Handle<Value> value) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    // Returns size >= required
    static size_t next_capacity(size_t required);

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
