#ifndef HAMMER_VM_INTERPRETER_HPP
#define HAMMER_VM_INTERPRETER_HPP

#include "hammer/vm/fwd.hpp"
#include "hammer/vm/objects/coroutine.hpp"
#include "hammer/vm/objects/value.hpp"

#include <array>

namespace hammer::vm {

/**
 * The interpreter is responsible for the creation and the execution
 * of coroutines.
 */
class Interpreter {
public:
    template<typename W>
    void walk(W&& w) {
        w(current_);

        for (byte i = 0; i < registers_used_; ++i) {
            w(registers_[i]);
        }
    }

    Coroutine create_coroutine(Context& ctx, Handle<Function> fn);
    Value run(Context& ctx, Handle<Coroutine> coro);

private:
    Value run_until_complete(Context& ctx, Handle<Coroutine> coro);
    void run_frame(Context& ctx, Handle<Coroutine> coro);

    template<typename T = Value>
    MutableHandle<T> reg(T initial) {
        Value* slot = allocate_register_slot();
        *slot = static_cast<Value>(initial);
        return MutableHandle<T>::from_slot(slot);
    }

private:
    Value* allocate_register_slot();

private:
    Coroutine current_;
    std::array<Value, 8> registers_{};
    byte registers_used_ = 0;
};

} // namespace hammer::vm

#endif // HAMMER_VM_INTERPRETER_HPP
