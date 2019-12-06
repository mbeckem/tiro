#ifndef HAMMER_VM_OBJECTS_FUNCTION_HPP
#define HAMMER_VM_OBJECTS_FUNCTION_HPP

#include "hammer/core/span.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/value.hpp"

#include <functional>

namespace hammer::vm {

/**
 * Represents executable byte code, typically used to 
 * represents the instructions within a function.
 * 
 * TODO: Need a bytecode validation routine.
 * TODO: Code should not be movable on the heap.
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
    void set(size_t index, Value value) const;

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

/**
 * A function where the first parameter ("this") has been bound
 * and will be automatically passed.
 */
class BoundMethod final : public Value {
public:
    static BoundMethod
    make(Context& ctx, Handle<Value> function, Handle<Value> object);

    BoundMethod() = default;

    explicit BoundMethod(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<BoundMethod>(), "Value is not a bound method.");
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

// TODO: NativeFunctions should reference the module they're defined in.
class NativeFunction final : public Value {
public:
    class Frame final {
    public:
        Context& ctx() const { return ctx_; }

        size_t arg_count() const { return args_.size(); }
        Handle<Value> arg(size_t index) const {
            HAMMER_CHECK(index < args_.size(),
                "NativeFunction::Frame::arg(): Index {} is out of bounds for "
                "argument count {}.",
                index, args_.size());
            return Handle<Value>::from_slot(&args_[index]);
        }

        void result(Value v) { result_slot_.set(v); }
        // TODO exceptions!

        Frame(const Frame&) = delete;
        Frame& operator=(const Frame&) = delete;

        explicit Frame(Context& ctx,
            Span<Value> args, // TODO Must be rooted!
            MutableHandle<Value> result_slot)
            : ctx_(ctx)
            , args_(args)
            , result_slot_(result_slot) {}

    private:
        Context& ctx_;
        Span<Value> args_;
        MutableHandle<Value> result_slot_;
    };

    using FunctionType = std::function<void(Frame& frame)>;

    static NativeFunction make(Context& ctx, Handle<String> name,
        u32 min_params, FunctionType function);

    NativeFunction() = default;

    explicit NativeFunction(Value v)
        : Value(v) {
        HAMMER_ASSERT(
            v.is<NativeFunction>(), "Value is not a native function.");
    }

    String name() const;
    u32 min_params() const;
    const FunctionType& function() const;

    // Called when collected.
    /// FIXME need real finalization architecture, don't call finalize on every object
    void finalize();

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    inline Data* access_heap() const;
};

// class AsyncFunction final : public Value {
// public:
//     using FunctionType = std::function<void()>; // TODO

//     static AsyncFunction make(Context& ctx, Handle<String> name, u32 min_params,
//         FunctionType function);
// };

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_FUNCTION_HPP
