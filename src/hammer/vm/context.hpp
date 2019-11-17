#ifndef HAMMER_VM_CONTEXT_HPP
#define HAMMER_VM_CONTEXT_HPP

#include "hammer/core/defs.hpp"
#include "hammer/vm/collector.hpp"
#include "hammer/vm/fwd.hpp"
#include "hammer/vm/heap.hpp"
#include "hammer/vm/interpreter.hpp"
#include "hammer/vm/objects/fwd.hpp"
#include "hammer/vm/objects/hash_table.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/types.hpp"

#include <string>
#include <unordered_map>

namespace hammer {

class CompiledModule;
class StringTable;

} // namespace hammer

namespace hammer::vm {

class Context final {
public:
    Context();
    ~Context();

    /// Attempts to register the given module with this context.
    /// Fails (i.e. returns false) if a module with that name has already been registered.
    bool add_module(Handle<Module> module);

    /// Attempts to find the module with the given name.
    /// Returns true on success and updates the module handle.
    bool find_module(Handle<String> name, MutableHandle<Module> module);

    /// Interns the given string, or returns an existing interned string that was previously interned.
    /// Interned strings can be compared using their addresses only.
    String intern_string(Handle<String> str);

    Value run(Handle<Function> fn);

    Heap& heap() { return heap_; }

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

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    template<typename W>
    inline void walk(W&& w);

    inline WriteBarrier write_barrier();

private:
    void intern_impl(MutableHandle<String> str,
        std::optional<MutableHandle<Symbol>> assoc_symbol);

private:
    friend class RootBase;

    // The Root class implements an intrusive stack using this pointer.
    RootBase** rooted_stack() { return &rooted_stack_; }

private:
    Heap heap_;
    Collector collector_;

    // This stack is used by the Root<T> to register their values.
    RootBase* rooted_stack_ = nullptr;

    Boolean true_;
    Boolean false_;
    Undefined undefined_;
    SpecialValue stop_iteration_;
    HashTable interned_strings_; // TODO this should eventually be a weak map
    HashTable modules_;

    Interpreter interpreter_;

    TypeSystem types_;
};

} // namespace hammer::vm

#endif // HAMMER_VM_CONTEXT_HPP
