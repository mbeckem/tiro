#ifndef HAMMER_VM_CONTEXT_HPP
#define HAMMER_VM_CONTEXT_HPP

#include "hammer/core/defs.hpp"
#include "hammer/vm/fwd.hpp"
#include "hammer/vm/heap/heap.hpp"
#include "hammer/vm/interpreter.hpp"
#include "hammer/vm/objects/fwd.hpp"
#include "hammer/vm/objects/hash_table.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/types.hpp"

// TODO think about boost or libuv or something else.
#include <boost/asio/ts/io_context.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace hammer {

class CompiledModule;
class StringTable;

} // namespace hammer

namespace hammer::vm {

class Context final {
public:
    Context();
    ~Context();

    Heap& heap() { return heap_; }

    boost::asio::io_context& io_context() { return io_context_; }

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

public:
    /// Execute the given function and return the result.
    Value run(Handle<Value> function);

private:
    // -- Functions responsible for scheduling and running coroutines

    void execute_coroutines();
    void schedule_coroutine(Handle<Coroutine> coro);
    Coroutine dequeue_coroutine();

private:
    // -- These functions are called by the frame types when resuming a waiting coroutine.
    friend NativeAsyncFunction::Frame;

    // Resumes a waiting coroutine.
    // The coroutine MUST be the in the "Waiting" state.
    void resume_coroutine(Handle<Coroutine> coro);

public:
    /// Attempts to register the given module with this context.
    /// Fails (i.e. returns false) if a module with that name has already been registered.
    bool add_module(Handle<Module> module);

    /// Attempts to find the module with the given name.
    /// Returns true on success and updates the module handle.
    bool find_module(Handle<String> name, MutableHandle<Module> module);

    /// Interns the given string, or returns an existing interned string that was previously interned.
    /// Interned strings can be compared using their addresses only.
    String intern_string(Handle<String> str);

    /// Returns the boolean object representing the given boolean value.
    /// The boolean object is a constant for this context.
    Boolean get_boolean(bool v) const noexcept { return v ? true_ : false_; }

    /// Returns the boolean object representing "true".
    /// The boolean object is a constant for this context.
    Boolean get_true() const noexcept { return true_; }

    /// Returns the boolean object representing "false".
    /// The boolean object is a constant for this context.
    Boolean get_false() const noexcept { return false_; }

    /// Returns the object representing the undefined value.
    /// This object is a constant for this context.
    Undefined get_undefined() const noexcept { return undefined_; }

    /// FIXME ugly
    SpecialValue get_stop_iteration() const noexcept { return stop_iteration_; }

    /// Returns a value that represents this integer. Integer values up to a certain
    /// limit can be packed into the value representation itself (without allocating any memory).
    Value get_integer(i64 value);

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

    // TODO: Move into interpreter, chose a better name?
    TypeSystem& types() { return types_; } // TODO

    template<typename W>
    inline void walk(W&& w);

private:
    void intern_impl(MutableHandle<String> str,
        std::optional<MutableHandle<Symbol>> assoc_symbol);

private:
    // -- These functions are called by the handle implementations to
    //    manage their value storage slots.

    friend RootBase;
    friend GlobalBase;

    // The Root class implements an intrusive stack using this pointer.
    RootBase** rooted_stack() { return &rooted_stack_; }

    // Registers the given global slot. The pointer MUST NOT be already registered.
    void register_global(Value* slot);

    // Unregisters a previously registered global slot. The slot MUST be registered.
    void unregister_global(Value* slot);

private:
    Heap heap_;

    // This stack is used by the Root<T> to register their values.
    RootBase* rooted_stack_ = nullptr;

    // This set is used to register global slots with arbitrary lifetime.
    // TODO: Better container.
    std::unordered_set<Value*> global_slots_;

    Boolean true_;
    Boolean false_;
    Undefined undefined_;
    SpecialValue stop_iteration_;
    Coroutine first_ready_, last_ready_; // Linked list of runnable coroutines
    HashTable interned_strings_; // TODO this should eventually be a weak map
    HashTable modules_;

    Interpreter interpreter_;

    TypeSystem types_;

    // True if some thread is currently running the io_context.
    bool running_ = false;

    // True if coroutines are currently executed. If this is false, the call to
    // resume_coroutine will start invoking ready coroutines (including the resumed one).
    // If this is true, nothing will happen because the "upper" executing function call
    // will run the new coroutine as well.
    bool coroutines_executing_ = false;

    // Should remain at the bottom here, the io_context's destructor
    // must execute before the other objects are destroyed.
    // For example, some handler in the io_context's loop may
    // contain Global<T> handles, which will unregister themselves
    // in the datastructures above.
    boost::asio::io_context io_context_;
};

} // namespace hammer::vm

#endif // HAMMER_VM_CONTEXT_HPP
