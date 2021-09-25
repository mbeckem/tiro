#ifndef TIRO_VM_ROOT_SET_IPP
#define TIRO_VM_ROOT_SET_IPP

#include "vm/root_set.hpp"

namespace tiro::vm {

template<typename Tracer>
void RootSet::trace(Tracer&& tracer) {
    // TODO The constant values should probably be allocated as "eternal",
    // so they will not have to be marked or traced.
    tracer(true_);
    tracer(false_);
    tracer(undefined_);
    tracer(first_ready_);
    tracer(last_ready_);
    tracer(interned_strings_);
    tracer(coroutines_);

    stack_.trace(tracer);
    externals_.trace(tracer);
    types_.trace(tracer);
    modules_.trace(tracer);
    interpreter_.trace(tracer);
}

} // namespace tiro::vm

#endif // TIRO_VM_ROOT_SET_IPP
