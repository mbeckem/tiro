#ifndef HAMMER_VM_CONTEXT_IPP
#define HAMMER_VM_CONTEXT_IPP

#include "hammer/vm/context.hpp"

#include "hammer/vm/handles.hpp"
#include "hammer/vm/object.hpp"

namespace hammer::vm {

template<typename W>
void Context::walk(W&& w) {
    // Walk the loaded modules
    for (auto& pair : modules_) {
        w(pair.second);
    }

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

    // TODO The constant values should probably be allocated as "eternal",
    // so they will not have to be traced.
    w(current_);
    w(true_);
    w(false_);
    w(thomb_);
    w(undefined_);

    for (auto& r : registers_)
        w(r);
}

WriteBarrier Context::write_barrier() {
    return WriteBarrier();
}

} // namespace hammer::vm

#endif // HAMMER_VM_CONTEXT_IPP
