#include "vm/handles/global.hpp"

#include "vm/context.hpp"

namespace tiro::vm {
namespace detail {

GlobalBase::GlobalBase(Context& ctx, Value initial)
    : ctx_(ctx)
    , slot_(initial) {
    ctx_.register_global(&slot_);
}

GlobalBase::~GlobalBase() {
    ctx_.unregister_global(&slot_);
}

} // namespace detail
} // namespace tiro::vm
