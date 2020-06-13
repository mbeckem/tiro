#ifndef TIRO_VM_OBJECTS_FUNCTIONS_HPP
#define TIRO_VM_OBJECTS_FUNCTIONS_HPP

#include "common/span.hpp"
#include "vm/heap/handles.hpp"
#include "vm/objects/tuples.hpp"
#include "vm/objects/value.hpp"

#include <functional>

namespace tiro::vm {

/// Represents executable byte code, typically used to
/// represents the instructions within a function.
///
/// TODO: Need a bytecode validation routine.
/// TODO: Code should not be movable on the heap.
class Code final : public Value {
public:
    static Code make(Context& ctx, Span<const byte> code);

    Code() = default;

    explicit Code(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<Code>(), "Value is not a code object.");
    }

    const byte* data() const;
    size_t size() const;
    Span<const byte> view() const { return {data(), size()}; }

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/// Represents a function prototype.
///
/// Function prototypes contain the static properties of functions and are referenced
/// by the actual function instances. Function prototypes are a necessary implementation
/// detail because actual functions (i.e. with closures) share all static properties
/// but have different closure variables each.
///
/// Function prototypes can be thought of as the 'class' of a function.
class FunctionTemplate final : public Value {
public:
    static FunctionTemplate make(Context& ctx, Handle<String> name, Handle<Module> module,
        u32 params, u32 locals, Span<const byte> code);

    FunctionTemplate() = default;

    explicit FunctionTemplate(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<FunctionTemplate>(), "Value is not a function template.");
    }

    String name() const;
    Module module() const;
    Code code() const;
    u32 params() const;
    u32 locals() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

/// Represents captured variables from an upper scope captured by a nested function.
/// ClosureContexts point to their parent (or null if they are at the root).
class Environment final : public Value {
public:
    static Environment make(Context& ctx, size_t size, Handle<Environment> parent);

    Environment() = default;

    explicit Environment(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<Environment>(), "Value is not a closure context.");
    }

    Environment parent() const;

    const Value* data() const;
    size_t size() const;
    Span<const Value> values() const { return {data(), size()}; }

    Value get(size_t index) const;
    void set(size_t index, Value value) const;

    // level == 0 -> return *this. Returns null in the unlikely case that the level is invalid.
    Environment parent(size_t level) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
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
class Function final : public Value {
public:
    static Function make(Context& ctx, Handle<FunctionTemplate> tmpl, Handle<Environment> closure);

    Function() = default;

    explicit Function(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<Function>(), "Value is not a function.");
    }

    FunctionTemplate tmpl() const;
    Environment closure() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

/// A function where the first parameter ("this") has been bound
/// and will be automatically passed as the first argument
/// of the wrapped function.
class BoundMethod final : public Value {
public:
    static BoundMethod make(Context& ctx, Handle<Value> function, Handle<Value> object);

    BoundMethod() = default;

    explicit BoundMethod(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<BoundMethod>(), "Value is not a bound method.");
    }

    Value function() const;
    Value object() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    inline Data* access_heap() const;
};

/// A sychronous native function. Useful for wrapping simple, nonblocking native APIs.
class NativeFunction final : public Value {
public:
    class Frame;

    using FunctionType = void (*)(Frame& frame);

    static NativeFunction make(
        Context& ctx, Handle<String> name, Handle<Tuple> values, u32 params, FunctionType function);

    NativeFunction() = default;

    explicit NativeFunction(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<NativeFunction>(), "Value is not a native function.");
    }

    String name() const;
    Tuple values() const;
    u32 params() const;
    FunctionType function() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    inline Data* access_heap() const;
};

class NativeFunction::Frame final {
public:
    Context& ctx() const { return ctx_; }

    Tuple values() const { return function_->values(); }

    size_t arg_count() const { return args_.size(); }
    Handle<Value> arg(size_t index) const {
        TIRO_CHECK(index < args_.size(),
            "NativeFunction::Frame::arg(): Index {} is out of bounds for "
            "argument count {}.",
            index, args_.size());
        return Handle<Value>::from_slot(&args_[index]);
    }

    void result(Value v) { result_slot_.set(v); }
    // TODO exceptions!

    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;

    explicit Frame(Context& ctx, Handle<NativeFunction> function,
        Span<Value> args, // TODO Must be rooted!
        MutableHandle<Value> result_slot)
        : ctx_(ctx)
        , function_(function)
        , args_(args)
        , result_slot_(result_slot) {}

private:
    Context& ctx_;
    Handle<NativeFunction> function_;
    Span<Value> args_;
    MutableHandle<Value> result_slot_;
};

/// Represents a native function that can be called to perform some async operation.
/// The coroutine will yield and wait until it is resumed by the async operation.
///
/// Note that calling functions of this type looks synchronous from the P.O.V. of
/// the user code.
class NativeAsyncFunction final : public Value {
public:
    class Frame;

    using FunctionType = void (*)(Frame frame);

    static NativeAsyncFunction make(
        Context& ctx, Handle<String> name, Handle<Tuple> values, u32 params, FunctionType function);

    NativeAsyncFunction() = default;

    explicit NativeAsyncFunction(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<NativeAsyncFunction>(), "Value is not a native async function.");
    }

    String name() const;
    Tuple values() const;
    u32 params() const;
    FunctionType function() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;

    inline Data* access_heap() const;
};

class NativeAsyncFunction::Frame final {
public:
    Context& ctx() const { return storage().coro_.ctx(); }

    Tuple values() const;

    size_t arg_count() const;
    Handle<Value> arg(size_t index) const;
    void result(Value v);
    // TODO exceptions!

    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;

    Frame(Frame&&) noexcept = default;
    Frame& operator=(Frame&&) noexcept = default;

    explicit Frame(Context& ctx, Handle<Coroutine> coro, Handle<NativeAsyncFunction> function,
        Span<Value> args, // TODO Must be rooted!
        MutableHandle<Value> result_slot);

    ~Frame();

private:
    struct Storage {
        Global<Coroutine> coro_;

        // Note: direct pointers into the stack. Only works because this kind of function
        // is a leaf function (no other functions will be called, therefore the stack will not
        // resize, therefore the pointers remain valid).
        // Note that the coroutine is being kept alive by the coro_ global handle above.
        Handle<NativeAsyncFunction> function_;
        Span<Value> args_;
        MutableHandle<Value> result_slot_;

        Storage(Context& ctx, Handle<Coroutine> coro, Handle<NativeAsyncFunction> function,
            Span<Value> args, MutableHandle<Value> result_slot);
    };

    Storage& storage() const {
        TIRO_DEBUG_ASSERT(storage_, "Invalid frame object (either moved or already resumed).");
        return *storage_;
    }

    // Schedules the coroutine for execution (after setting the return value).
    void resume();

    // TODO allocator
    std::unique_ptr<Storage> storage_;
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_FUNCTIONS_HPP
