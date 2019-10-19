#ifndef HAMMER_VM_CONTEXT_HPP
#define HAMMER_VM_CONTEXT_HPP

#include "hammer/core/defs.hpp"
#include "hammer/vm/collector.hpp"
#include "hammer/vm/coroutine.hpp"
#include "hammer/vm/heap.hpp"
#include "hammer/vm/object.hpp"

#include <string>
#include <unordered_map>

namespace hammer {

class CompiledModule;
class StringTable;

} // namespace hammer

namespace hammer::vm {

class Boolean;
class Integer;
class String;
class Coroutine;

class WriteBarrier;
class RootBase;

class Context final {
public:
    Context();
    ~Context();

    // TODO: Load many modules, execute just one of them.
    Module load(const CompiledModule& module, const StringTable& strings);
    Value run(Handle<Function> fn);

    Heap& heap() { return heap_; }

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    template<typename W>
    inline void walk(W&& w);

    inline WriteBarrier write_barrier();

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
    // TODO this should be on the gc heap.
    std::unordered_map<std::string, Module> modules_;

    Heap heap_;
    Collector collector_;

    // This stack is used by the Root<T> to register their values.
    RootBase* rooted_stack_ = nullptr;

    Coroutine current_;
    Boolean true_;
    Boolean false_;
    Undefined undefined_;

    std::array<Value, 8> registers_{};
};

} // namespace hammer::vm

#endif // HAMMER_VM_CONTEXT_HPP
