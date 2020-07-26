#include "vm/handles/frame.hpp"

#include "common/math.hpp"
#include "common/scope.hpp"

#include <new>

namespace tiro::vm {

// Assumption during deallocation.
static_assert(std::is_trivially_destructible_v<Frame>);

FrameCollection::FrameCollection() {}

FrameCollection::~FrameCollection() {
    for (Frame* frame : frames_)
        ::operator delete(static_cast<void*>(frame));
}

Frame* FrameCollection::allocate_frame(size_t slots) {
    size_t slots_size = slots;
    if (!checked_mul(slots_size, sizeof(Value)))
        throw std::bad_alloc();

    size_t frame_size = slots_size;
    if (!checked_add(frame_size, sizeof(Frame)))
        throw std::bad_alloc();

    void* storage = ::operator new(frame_size);
    ScopeExit guard = [&] { ::operator delete(storage); };

    Frame* frame = new (storage) Frame(this, slots);
    [[maybe_unused]] bool inserted = frames_.insert(frame).second;
    TIRO_DEBUG_ASSERT(inserted, "Insertion of new frame must be successful.");

    guard.disable();
    return frame;
}

void FrameCollection::destroy_frame(Frame* frame) noexcept {
    if (!frame)
        return;

    TIRO_DEBUG_ASSERT(&frame->collection() == this, "Frame must belong to this collection.");
    TIRO_DEBUG_ASSERT(
        frames_.count(frame) > 0, "Frame must have been registered with the collection.");

    frames_.erase(frame);
    ::operator delete(static_cast<void*>(frame));
}

Frame::Frame(FrameCollection* collection, size_t slot_count)
    : collection_(collection)
    , slot_count_(slot_count) {
    std::uninitialized_fill_n(slots_, slot_count_, Value());
}

} // namespace tiro::vm
