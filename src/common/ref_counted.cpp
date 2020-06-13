#include "common/ref_counted.hpp"

namespace tiro {

RefCounted::~RefCounted() {
    TIRO_DEBUG_ASSERT(refcount_ == 0, "Must not be deleted if refcount > 0.");
    if (weak_) {
        weak_->self = nullptr;
        weak_->dec_ref();
    }
}

RefCounted::RefCounted()
    : weak_(nullptr)
    , refcount_(1) {}

void RefCounted::inc_ref() {
    refcount_++;
}

void RefCounted::dec_ref() {
    TIRO_DEBUG_ASSERT(refcount_ > 0, "Invalid refcount (must be greater than zero).");
    if (--refcount_ == 0)
        delete this;
}

RefCounted::WeakData* RefCounted::weak_ref() {
    if (!weak_) {
        weak_ = new WeakData;
        weak_->self = this;
        weak_->refcount = 1;
    }
    return weak_;
}

void RefCounted::WeakData::inc_ref() {
    ++refcount;
}

void RefCounted::WeakData::dec_ref() {
    TIRO_DEBUG_ASSERT(refcount > 0, "Invalid refcount (must be greater than zero).");
    if (--refcount == 0)
        delete this;
}

} // namespace tiro
