#ifndef TIRO_VM_HEAP_NEW_COLLECTOR_HPP
#define TIRO_VM_HEAP_NEW_COLLECTOR_HPP

#include "common/defs.hpp"
#include "vm/fwd.hpp"
#include "vm/heap/fwd.hpp"

#include <vector>

namespace tiro::vm::new_heap {

/// Represents the reason for a garbage collection cycle.
enum class GcReason {
    /// collection was triggered automatically (threshold etc.)
    Automatic,

    /// forced collection
    Forced,

    /// triggered by previous allocation failure
    AllocFailure,
};

std::string_view to_string(GcReason reason);

class Collector final {
public:
    Collector();
    ~Collector();

    Collector(const Collector&) = delete;
    Collector& operator=(const Collector&) = delete;

    /// Collects garbage.
    /// Traces the heap by following references in `roots`.
    /// After tracing is complete, sweeps free space in `heap`.
    void collect(RootSet& roots, Heap& heap, GcReason reason);

    /// Heap size (in bytes) at which the garbage collector should be invoked again.
    /// TODO: Introduce another automatic trigger (such as elapsed time since last gc).
    /// TODO: This is pretty naive.
    size_t next_threshold() const noexcept { return next_threshold_; }

private:
    class Tracer;

    /// Entry point called when tracing starts.
    void trace(RootSet& roots);

    /// Called for every object reference seen while tracing.
    /// If the value has not been traced yet, it will be placed onto the trace stack.
    void mark(Value value);

    /// Actually trace the value, visiting all directly reachable values.
    /// Called at most once for every object.
    void trace_value(Value value, Tracer& tracer);

private:
    void sweep(Heap& heap);

private:
    bool running_ = false;

    // For marking. Should be replaced by some preallocated memory in the future.
    std::vector<Value> to_trace_;

    // Duration of last gc, in milliseconds.
    double last_duration_ = 0;

    // Next automatic gc call (byte threshold).
    size_t next_threshold_ = size_t(1) << 20;
};

} // namespace tiro::vm::new_heap

#endif // TIRO_VM_HEAP_NEW_COLLECTOR_HPP
