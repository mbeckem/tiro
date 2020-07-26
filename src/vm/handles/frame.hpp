#ifndef TIRO_VM_HANDLES_FRAMES_HPP
#define TIRO_VM_HANDLES_FRAMES_HPP

#include "common/defs.hpp"
#include "vm/handles/fwd.hpp"
#include "vm/handles/handle.hpp"

#include "absl/container/flat_hash_set.h"

#include <memory>

namespace tiro::vm {

/// A frame collection is a container for variable sized frames.
///
/// Frames can be allocated and deallocated through this class. Values stored in a frame are rooted (i.e.
/// they are known to the garbage collector).
///
/// Note that frames are less efficient to the superious `Scope` and `Local` facilities. However, frames
/// have dynamic lifetime whereas Scopes must be used as a stack.
///
/// The main use case of frames (for now) is the external API, where pointers to frames a handed to native code.
class FrameCollection final {
public:
    struct FrameDeleter {
        inline void operator()(Frame* frame) noexcept;
    };

    using FramePtr = std::unique_ptr<Frame, FrameDeleter>;

    FrameCollection();
    ~FrameCollection();

    FrameCollection(const FrameCollection&) = delete;
    FrameCollection& operator=(const FrameCollection&) = delete;

    /// Convenience function that returns a frame pointer wrapped inside a std::unique_ptr
    /// that will destroy the frame automatically.
    FramePtr create_frame(size_t slots) { return FramePtr(allocate_frame(slots), FrameDeleter{}); }

    /// Allocates a new frame of the given number of slots.
    /// The frame is registered with the collection and will remain valid
    /// until destroyed by the user.
    /// Call `destroy_frame` in order to destroy and unregister a frame.
    ///
    /// Frames must not outlive the frame collection that created them.
    Frame* allocate_frame(size_t slots);

    /// Unregisters, destroys and deallocates the given frame.
    /// Equivalent to `frame->destroy()`. Note that the frame must have
    /// actually be created by this collection.
    void destroy_frame(Frame* frame) noexcept;

    /// Returns the total number of registers frames.
    size_t frame_count() const { return frames_.size(); }

    template<typename Tracer>
    inline void trace(Tracer&& tracer);

private:
    absl::flat_hash_set<Frame*> frames_;
};

// Improvement: HandleSpan ops if this class is used inside the library? The minimal interface
// is fine for as long as this is only used by c code.
class Frame final {
public:
    /// Destroys this frame. The instance will be deleted!
    void destroy() { return collection().destroy_frame(this); }

    /// Returns the collection that allocated this frame.
    FrameCollection& collection() const { return *collection_; }

    /// Returns the slot address with the given index.
    Value* slot(size_t index) {
        TIRO_DEBUG_ASSERT(index < size(), "Slot index out of bounds.");
        return slots_ + index;
    }

    /// Returns the number of slots in this frame.
    size_t size() const { return slot_count_; }

    Span<Value> raw_slots() { return Span<Value>(slots_, slot_count_); }

private:
    friend FrameCollection;

    explicit Frame(FrameCollection* collection, size_t slot_count);

    void operator delete(void*) = delete;

private:
    FrameCollection* collection_;
    size_t slot_count_;
    Value slots_[];
};

void FrameCollection::FrameDeleter::operator()(Frame* frame) noexcept {
    frame->destroy();
};

template<typename Tracer>
inline void FrameCollection::trace(Tracer&& tracer) {
    for (Frame* frame : frames_) {
        tracer(frame->raw_slots());
    }
}

} // namespace tiro::vm

#endif // TIRO_VM_HANDLES_FRAMES_HPP
