#ifndef TIRO_VM_CONTEXT_IPP
#define TIRO_VM_CONTEXT_IPP

#include "vm/context.hpp"

namespace tiro::vm {

template<typename Tracer>
void Context::trace(Tracer&& tracer) {
    // Walk all global slots
    for (Value* v : global_slots_) {
        tracer(*v);
    }

    // TODO The constant values should probably be allocated as "eternal",
    // so they will not have to be marked or traced.
    tracer(true_);
    tracer(false_);
    tracer(undefined_);
    tracer(stop_iteration_);
    tracer(first_ready_);
    tracer(last_ready_);
    tracer(interned_strings_);
    tracer(modules_);

    stack_.trace(tracer);
    frames_.trace(tracer);
    types_.trace(tracer);
    interpreter_.trace(tracer);
}

} // namespace tiro::vm

#endif // TIRO_VM_CONTEXT_IPP
