#ifndef HAMMER_VM_CONTEXT_IPP
#define HAMMER_VM_CONTEXT_IPP

#include "hammer/vm/context.hpp"

#include "hammer/vm/heap/handles.hpp"

namespace hammer::vm {

template<typename W>
void Context::walk(W&& w) {
    // Walk the stack of rooted values
    {
        RootBase* r = rooted_stack_;
        while (r) {
            HAMMER_ASSERT(r->stack_ == &this->rooted_stack_,
                "Invalid stack top pointer.");
            w(r->slot_);
            r = r->prev_;
        }
    }

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

    types_.walk(w);
    interpreter_.walk(w);
}

} // namespace hammer::vm

#endif // HAMMER_VM_CONTEXT_IPP
