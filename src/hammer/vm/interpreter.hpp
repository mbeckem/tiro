#ifndef HAMMER_VM_INTERPRETER_HPP
#define HAMMER_VM_INTERPRETER_HPP

#include "hammer/vm/fwd.hpp"
#include "hammer/vm/objects/coroutine.hpp"
#include "hammer/vm/objects/value.hpp"

#include <array>

namespace hammer::vm {

class Interpreter {
public:
    template<typename W>
    void walk(W&& w) {
        w(current_);
        for (auto& r : registers_) {
            w(r);
        }
    }

    Coroutine create_coroutine(Context& ctx, Handle<Function> fn);
    Value run(Context& ctx, Handle<Coroutine> coro);

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

        Handle<T> handle() { return Handle<T>::from_slot(slot_); }

        MutableHandle<T> mut_handle() {
            return MutableHandle<T>::from_slot(slot_);
        }

        register_slot(const register_slot&) = delete;
        register_slot& operator=(const register_slot&) = delete;
        register_slot(register_slot&&) = delete;
        register_slot& operator=(register_slot&&) = delete;
    };

    Value run_until_complete(Context& ctx, Handle<Coroutine> coro);
    void run_frame(Context& ctx, Handle<Coroutine> coro);

    template<size_t Index, typename T = Value>
    register_slot<T> reg() {
        return {&std::get<Index>(registers_)};
    }

    template<size_t Index, typename T = Value>
    register_slot<T> reg(T initial) {
        return {&std::get<Index>(registers_), initial};
    }

private:
    Coroutine current_;
    std::array<Value, 8> registers_{};
};

} // namespace hammer::vm

#endif // HAMMER_VM_INTERPRETER_HPP
