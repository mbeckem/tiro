#ifndef HAMMER_VM_CONTEXT_HPP
#define HAMMER_VM_CONTEXT_HPP

#include "hammer/core/defs.hpp"
#include "hammer/vm/collector.hpp"
#include "hammer/vm/fwd.hpp"
#include "hammer/vm/heap.hpp"
#include "hammer/vm/objects/coroutine.hpp"
#include "hammer/vm/objects/fwd.hpp"
#include "hammer/vm/objects/hash_table.hpp"
#include "hammer/vm/objects/object.hpp"

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

    Boolean get_boolean(bool v) const noexcept { return v ? true_ : false_; }
    Undefined get_undefined() const noexcept { return undefined_; }
    SpecialValue get_stop_iteration() const noexcept { return stop_iteration_; }

    /// Warning: the string view be stable in memory, as the function might allocate.
    String get_interned_string(std::string_view value);

    /// Returns a symbol with the given name. Symbols are unique in memory.
    Symbol get_symbol(Handle<String> str);

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    template<typename W>
    inline void walk(W&& w);

    inline WriteBarrier write_barrier();

private:
    void intern_impl(MutableHandle<String> str,
        std::optional<MutableHandle<Symbol>> assoc_symbol);

private:
    // Uses one of the registers (Context->registers_) as a typed slot.
    // The destructor clears the register.
    template<typename T = Value>
    struct register_slot : PointerOps<T, register_slot<T>> {
    private:
        Value* slot_;

    public:
        register_slot(Value* slot)
            : slot_(slot) {}

        register_slot(Value* slot, T initial)
            : slot_(slot) {
            *slot_ = initial;
        }

        // Clear for the next use; dont keep objects rooted.
        ~register_slot() { *slot_ = Value(); }

        void set(T value) { *slot_ = static_cast<Value>(value); }
        T get() { return static_cast<T>(*slot_); }
        /* implicit */ operator T() { return get(); }
        /* implicit */ operator Handle<T>() {
            return Handle<T>::from_slot(slot_);
        }

        register_slot(const register_slot&) = delete;
        register_slot& operator=(const register_slot&) = delete;
        register_slot(register_slot&&) = delete;
        register_slot& operator=(register_slot&&) = delete;
    };

private:
    friend class RootBase;

    // The Root class implements an intrusive stack using this pointer.
    RootBase** rooted_stack() { return &rooted_stack_; }

private:
    Value run_until_complete(Handle<Coroutine> coro);
    void run_frame(Handle<Coroutine> coro);

    template<size_t Index, typename T = Value>
    register_slot<T> reg() {
        return {&std::get<Index>(registers_)};
    }

    template<size_t Index, typename T = Value>
    register_slot<T> reg(T initial) {
        return {&std::get<Index>(registers_), initial};
    }

private:
    Heap heap_;
    Collector collector_;

    // This stack is used by the Root<T> to register their values.
    RootBase* rooted_stack_ = nullptr;

    Coroutine current_;
    Boolean true_;
    Boolean false_;
    Undefined undefined_;
    SpecialValue stop_iteration_;
    HashTable interned_strings_; // TODO this should eventually be a weak map
    HashTable modules_;

    std::array<Value, 8> registers_{};
};

} // namespace hammer::vm

#endif // HAMMER_VM_CONTEXT_HPP
