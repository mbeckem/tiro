#ifndef TIRO_VM_CONTEXT_IPP
#define TIRO_VM_CONTEXT_IPP

#include "vm/context.hpp"

namespace tiro::vm {

template<typename W>
void Context::walk(W&& w) {
    // Walk all global slots
    for (Value* v : global_slots_) {
        w(*v);
    }

    // TODO The constant values should probably be allocated as "eternal",
    // so they will not have to be marked or traced.
    w(true_);
    w(false_);
    w(undefined_);
    w(stop_iteration_);
    w(first_ready_);
    w(last_ready_);
    w(interned_strings_);
    w(modules_);

    stack_.trace(w);
    types_.trace(w);
    interpreter_.trace(w);
}

} // namespace tiro::vm

#endif // TIRO_VM_CONTEXT_IPP
