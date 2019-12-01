#ifndef HAMMER_VM_COLLECTOR_HPP
#define HAMMER_VM_COLLECTOR_HPP

#include "hammer/core/defs.hpp"
#include "hammer/vm/objects/value.hpp"

#include <vector>

namespace hammer::vm {

class Context;

enum class GcTrigger {
    Automatic,
    Forced,
    AllocFailure,
};

std::string_view to_string(GcTrigger trigger);

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

} // namespace hammer::vm

#endif // HAMMER_VM_COLLECTOR_HPP
