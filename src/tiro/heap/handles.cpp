#include "tiro/heap/handles.hpp"

#include "tiro/vm/context.hpp"

namespace tiro::vm {

RootBase::RootBase(Context& ctx, Value value)
    : stack_(ctx.rooted_stack())
    , prev_(*stack_)
    , slot_(value) {
    *stack_ = this;
}

RootBase::~RootBase() {
    TIRO_ASSERT(
        *stack_ == this, "Root object used in a non stack like fashion.");
    *stack_ = prev_;
}

GlobalBase::GlobalBase(Context& ctx, Value value)
    : ctx_(ctx)
    , slot_(value) {
    ctx_.register_global(&slot_);
}

GlobalBase::~GlobalBase() {
    ctx_.unregister_global(&slot_);
}

} // namespace tiro::vm
