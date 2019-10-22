#include "hammer/vm/handles.hpp"

#include "hammer/vm/context.hpp"

namespace hammer::vm {

RootBase::RootBase(Context& ctx, Value value)
    : stack_(ctx.rooted_stack())
    , prev_(*stack_)
    , slot_(value) {
    *stack_ = this;
}

RootBase::~RootBase() {
    HAMMER_ASSERT(
        *stack_ == this, "Root object used in a non stack like fashion.");
    *stack_ = prev_;
}

} // namespace hammer::vm
