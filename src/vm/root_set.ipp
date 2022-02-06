#ifndef TIRO_VM_ROOT_SET_IPP
#define TIRO_VM_ROOT_SET_IPP

#include "vm/root_set.hpp"

namespace tiro::vm {

template<typename Tracer>
void RootSet::trace(Tracer&& tracer) {
    tracer(true_);
    tracer(false_);
    tracer(undefined_);
    tracer(first_ready_);
    tracer(last_ready_);
    tracer(interned_strings_);

    stack_.trace(tracer);
    externals_.trace(tracer);
    types_.trace(tracer);
    modules_.trace(tracer);
    interpreter_.trace(tracer);
}

} // namespace tiro::vm

#endif // TIRO_VM_ROOT_SET_IPP
