#ifndef TIRO_VM_HEAP_COLLECTOR_HPP
#define TIRO_VM_HEAP_COLLECTOR_HPP

#include "tiro/core/defs.hpp"
#include "tiro/vm/objects/value.hpp"

#include <vector>

namespace tiro::vm {

class Context;

/// Represents the reason for a garbage collection cycle.
enum class GcTrigger {
    Automatic,
    Forced,
    AllocFailure,
};

std::string_view to_string(GcTrigger trigger);

/// Implements garbage collection.
///
/// All values that are in use by a Context must be reachable by the garbage collector
/// in order to be marked as "live". Dead objects are free'd as part of the collection
/// cycle.
class Collector final {
public:
    Collector();

    Collector(const Collector&) = delete;
    Collector& operator=(const Collector&) = delete;

    /// Invoke the garbage collector. Traces the complete heap
    /// and frees objects that are no longer referenced.
    void collect(Context& ctx, GcTrigger trigger);

    /// Heap size (in bytes) at which the garbage collector should be invoked again.
    /// TODO: Introduce another automatic trigger (such as elapsed time since last gc).
    size_t next_threshold() const noexcept { return next_threshold_; }

private:
    struct Walker;

private:
    void trace_heap(Context& ctx);
    void sweep_heap(Context& ctx);

    void mark(Value v);
    void trace(Walker& w, Value v);

    static size_t
    compute_next_threshold(size_t last_threshold, size_t current_heap_size);

private:
    // For marking. Should be replaced by some preallocated memory in the future.
    std::vector<Value> to_trace_;

    // Duration of last gc, in milliseconds.
    double last_duration_ = 0;

    // Next automatic gc call (byte threshold).
    size_t next_threshold_ = size_t(1) << 20;
};

} // namespace tiro::vm

#endif // TIRO_VM_HEAP_COLLECTOR_HPP
