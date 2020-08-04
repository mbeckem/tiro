#ifndef TIRO_VM_CONTEXT_HPP
#define TIRO_VM_CONTEXT_HPP

#include "common/defs.hpp"
#include "vm/fwd.hpp"
#include "vm/handles/frame.hpp"
#include "vm/handles/fwd.hpp"
#include "vm/handles/scope.hpp"
#include "vm/heap/heap.hpp"
#include "vm/interpreter.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/fwd.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/primitives.hpp"
#include "vm/types.hpp"

#include <asio/ts/io_context.hpp>

#include "absl/container/flat_hash_set.h"

#include <string>

namespace tiro {

class StringTable;

} // namespace tiro

namespace tiro::vm {

class Context final {
public:
    Context();
    ~Context();

    Heap& heap() { return heap_; }

    asio::io_context& io_context() { return io_context_; }

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

public:
    /// Execute the given function and return the result.
    /// This function blocks until the function completes.
    /// The values in the arguments tuple (if not null) will be passed
    /// to the function.
    Value run(Handle<Value> function, MaybeHandle<Tuple> arguments);

    /// The timestamp of the current main loop iteration, i.e.
    /// when the main loop woke up to execute ready coroutines.
    ///
    /// The timestamp's unit is milliseconds.
    /// The values increases monotonically, starting from some *arbitrary* time point.
    ///
    /// All user code executed from within the same main loop iteration
    /// will observe the same timestamp, so this is not a precise
    /// tool to measure elapsed time.
    i64 loop_timestamp() const { return loop_timestamp_; }

    RootedStack& stack() { return stack_; }
    FrameCollection& frames() { return frames_; }

private:
    // -- Functions responsible for scheduling and running coroutines

    void execute_coroutines();
    void schedule_coroutine(Handle<Coroutine> coro);
    Nullable<Coroutine> dequeue_coroutine();

private:
    // -- These functions are called by the frame types when resuming a waiting coroutine.
    friend NativeAsyncFunctionFrame;

    // Resumes a waiting coroutine.
    // The coroutine MUST be the in the "Waiting" state.
    void resume_coroutine(Handle<Coroutine> coro);

public:
    /// Attempts to register the given module with this context.
    /// Fails (i.e. returns false) if a module with that name has already been registered.
    bool add_module(Handle<Module> module);

    /// Attempts to find the module with the given name.
    /// Returns true on success and updates the module handle.
    bool find_module(Handle<String> name, OutHandle<Module> module);

    /// Returns true if the value is considered as true in boolean contexts.
    bool is_truthy(Handle<Value> v) const noexcept { return !(v->is_null() || v->same(false_)); }

    /// Returns the boolean object representing the given boolean value.
    /// The boolean object is a constant for this context.
    Boolean get_boolean(bool v) const noexcept { return v ? true_.value() : false_.value(); }

    /// Returns the boolean object representing "true".
    /// The boolean object is a constant for this context.
    Boolean get_true() const noexcept { return true_.value(); }

    /// Returns the boolean object representing "false".
    /// The boolean object is a constant for this context.
    Boolean get_false() const noexcept { return false_.value(); }

    /// Returns the object representing the undefined value.
    /// This object is a constant for this context.
    Undefined get_undefined() const noexcept { return undefined_.value(); }

    /// Returns a value that represents this integer. Integer values up to a certain
    /// limit can be packed into the value representation itself (without allocating any memory).
    Value get_integer(i64 value);

    /// Interns the given string, or returns an existing interned string that was previously interned.
    /// Interned strings can be compared using their addresses only.
    String get_interned_string(Handle<String> str);

    /// Returns a string object with the given content.
    /// The string object is interned (i.e. deduplicated)
    ///
    /// Warning: the string view must be stable in memory, as the function might allocate.
    String get_interned_string(std::string_view value);

    /// Returns a symbol with the given name.
    Symbol get_symbol(Handle<String> str);

    /// Returns a symbol with the given name.
    /// Warning: the string view be stable in memory, as the function might allocate.
    Symbol get_symbol(std::string_view value);

    /// Returns a new coroutine and schedules it for execution.
    /// The function `func` will be invoked with the given arguments tuple
    /// from the new coroutine. The arguments tuple may be null (for 0 arguments).
    Coroutine make_coroutine(Handle<Value> func, MaybeHandle<Tuple> arguments);

    TypeSystem& types() { return types_; }

    template<typename Tracer>
    inline void trace(Tracer&& tracer);

private:
    void intern_impl(MutHandle<String> str, MaybeOutHandle<Symbol> assoc_symbol);

private:
    // -- These functions are called by the handle implementations to
    //    manage their value storage slots.

    friend detail::GlobalBase;

    // Registers the given global slot. The pointer MUST NOT be already registered.
    void register_global(Value* slot);

    // Unregisters a previously registered global slot. The slot MUST be registered.
    void unregister_global(Value* slot);

private:
    // This set is used to register global slots with arbitrary lifetime.
    absl::flat_hash_set<Value*> global_slots_;

    // The context must survive everything except for the roots.
    // Objects on the heap (below) may have finalizers that reference the io context, so it must
    // not be destroyed before the heap. When the io context is destroyed, only destructors
    // of handlers will run - those may have globals that will deregister themselves
    // from the root set, so the io context must not be destroyed before the global slots above.
    //
    // An alternative would be do resolve the order conflict differently, e.g. by keeping the heap
    // alive but running all finalizers at the start of the shutdown procedure.
    asio::io_context io_context_;

    Heap heap_;

    // TODO No need to make this nullable - wrap it into a root set type.
    Nullable<Boolean> true_;
    Nullable<Boolean> false_;
    Nullable<Undefined> undefined_;
    Nullable<Coroutine> first_ready_, last_ready_; // Linked list of runnable coroutines
    Nullable<HashTable> interned_strings_;         // TODO this should eventually be a weak map
    Nullable<HashTable> modules_;

    RootedStack stack_;
    FrameCollection frames_;
    Interpreter interpreter_;
    TypeSystem types_;

    // True if some thread is currently running the io_context.
    bool running_ = false;

    // True if coroutines are currently executed. If this is false, the call to
    // resume_coroutine will start invoking ready coroutines (including the resumed one).
    // If this is true, nothing will happen because the "upper" executing function call
    // will run the new coroutine as well.
    bool coroutines_executing_ = false;

    // steady time at context construction
    i64 startup_time_ = 0;

    // current steady time - startup steady time
    i64 loop_timestamp_ = 0;
};

} // namespace tiro::vm

#endif // TIRO_VM_CONTEXT_HPP
