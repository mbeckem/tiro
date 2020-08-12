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

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

public:
    /// Creates a new coroutine that will execute the given `func` object with `args` as its function arguements.
    /// Note that the coroutine will not begin execution before it is passed to `start()`.
    Coroutine make_coroutine(Handle<Value> func, MaybeHandle<Tuple> args);

    /// Sets the given callback function as the native callback for `coro`. The callback will be executed
    /// when the coroutine completes (with a result or with an error). The callback's destructor will always
    /// run, even if the callback itself was not triggered (for example, if the context is destroyed before `coro` completes).
    void
    set_callback(Handle<Coroutine> coro, std::function<void(MutHandle<Coroutine>)> on_complete);

    /// Schedules the initial execution of the given coroutine. The coroutine will be executed from within the
    /// next call to one of the run functions. The coroutine must be in its initial state.
    void start(Handle<Coroutine> coro);

    /// Runs all coroutines that have been scheduled for execution. Stops when all coroutines have either completed
    /// or yielded (waiting for async operations to complete). When a coroutine has completed its execution, its native
    /// callback will be executed (if one has been specified in `set_callback()`).
    void run_ready();

    /// Returns true if there is at least one coroutine ready for execution.
    bool has_ready() const;

    /// Executes a module initialization function. Does not support yielding (i.e. all calls must be synchronoous).
    /// TODO: Support for async module initializers, just use the same code path as the normal code.
    /// Not yet implemented because of other priorities.
    Value run_init(Handle<Value> func, MaybeHandle<Tuple> args);

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
    // -- Functions responsible for scheduling coroutines.
    friend Interpreter;

    void schedule_coroutine(Handle<Coroutine> coro);
    Nullable<Coroutine> dequeue_coroutine();
    void execute_callbacks(Handle<Coroutine> coro);

private:
    // -- These functions are called by the frame types when resuming a waiting coroutine.
    friend NativeAsyncFunctionFrame;

    // Resumes a waiting coroutine.
    // The coroutine MUST be the in the "Running" or "Waiting" state.
    // "Running" if already running and immediately resumed (enqueue ready coro at the end to yield).
    // "Waiting" if coro was suspended and will now continue execution.
    void resume_coroutine(Handle<Coroutine> coro);

public:
    /// Attempts to register the given module with this context.
    /// Fails (i.e. returns false) if a module with that name has already been registered.
    bool add_module(Handle<Module> module);

    /// Attempts to find the module with the given name.
    /// Returns true on success and updates the module handle.
    bool find_module(Handle<String> name, OutHandle<Module> module);

    /// Returns true if the value is considered as true in boolean contexts.
    bool is_truthy(Handle<Value> v) const noexcept { return is_truthy(*v); }
    bool is_truthy(Value v) const noexcept { return !(v.is_null() || v.same(false_)); }

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
    /// Warning: the string view must be stable in memory, as the function might allocate.
    Symbol get_symbol(std::string_view value);

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

    Heap heap_;

    // TODO No need to make this nullable - wrap it into a root set type.
    Nullable<Boolean> true_;
    Nullable<Boolean> false_;
    Nullable<Undefined> undefined_;
    Nullable<Coroutine> first_ready_, last_ready_; // Linked list of runnable coroutines
    Nullable<HashTable> interned_strings_;         // TODO this should eventually be a weak map
    Nullable<HashTable> modules_;

    // Created and not yet completed coroutines.
    Nullable<Set> coroutines_;

    RootedStack stack_;
    FrameCollection frames_;
    Interpreter interpreter_;
    TypeSystem types_;

    // True if some thread is currently running.
    bool running_ = false;

    // steady time at context construction
    i64 startup_time_ = 0;

    // current steady time - startup steady time
    i64 loop_timestamp_ = 0;
};

} // namespace tiro::vm

#endif // TIRO_VM_CONTEXT_HPP
