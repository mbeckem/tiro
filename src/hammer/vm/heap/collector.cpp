#include "hammer/vm/heap/collector.hpp"

#include "hammer/core/span.hpp"
#include "hammer/vm/context.hpp"
#include "hammer/vm/heap/heap.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/raw_arrays.hpp"
#include "hammer/vm/objects/small_integer.hpp"

#include "hammer/vm/context.ipp"
#include "hammer/vm/objects/array.ipp"
#include "hammer/vm/objects/classes.ipp"
#include "hammer/vm/objects/coroutine.ipp"
#include "hammer/vm/objects/function.ipp"
#include "hammer/vm/objects/hash_table.ipp"
#include "hammer/vm/objects/native_object.ipp"
#include "hammer/vm/objects/object.ipp"
#include "hammer/vm/objects/string.ipp"

#include <chrono>

// Enable debug logging
// #define HAMMER_TRACE_GC_ENABLED

#ifdef HAMMER_TRACE_GC_ENABLED
#    include <iostream>
#    define HAMMER_TRACE_GC(...) \
        (std::cout << "Collector: " << fmt::format(__VA_ARGS__) << std::endl)
#else
#    define HAMMER_TRACE_GC(...)
#endif

namespace hammer::vm {

template<typename TimePoint>
static double elapsed_ms(TimePoint start, TimePoint end) {
    std::chrono::duration<double, std::milli> millis = end - start;
    return millis.count();
}

std::string_view to_string(GcTrigger trigger) {
    switch (trigger) {
    case GcTrigger::Automatic:
        return "Automatic";
    case GcTrigger::Forced:
        return "Forced";
    case GcTrigger::AllocFailure:
        return "AllocFailure";
    }

    HAMMER_UNREACHABLE("Invalid trigger value.");
}

struct Collector::Walker {
    Collector* gc;

    void operator()(Value& v) { gc->mark(v); }

    void operator()(HashTableEntry& e) { e.walk(*this); }

    template<typename T>
    void array(ArrayVisitor<T> array) {
        // TODO dont visit all members of an array at once, instead
        // push the visitor itself on the stack.
        while (array.has_item()) {
            operator()(array.get_item());
            array.advance();
        }
    }
};

Collector::Collector() {}

void Collector::collect(Context& ctx, [[maybe_unused]] GcTrigger trigger) {
    HAMMER_ASSERT(this == &ctx.heap().collector(),
        "Collector does not belong to this context.");

    [[maybe_unused]] const size_t size_before_collect =
        ctx.heap().allocated_bytes();
    [[maybe_unused]] const size_t objects_before_collect =
        ctx.heap().allocated_objects();

    HAMMER_TRACE_GC(
        "Invoking collect() at heap size {} ({} objects). Trigger: {}.",
        size_before_collect, objects_before_collect, to_string(trigger));

    const auto start = std::chrono::steady_clock::now();
    {
        trace_heap(ctx);
        sweep_heap(ctx);
    }
    const auto end = std::chrono::steady_clock::now();
    const auto duration = elapsed_ms(start, end);

    const size_t size_after_collect = ctx.heap().allocated_bytes();
    [[maybe_unused]] const size_t objects_after_collect =
        ctx.heap().allocated_objects();

    last_duration_ = duration;
    next_threshold_ = compute_next_threshold(
        next_threshold_, size_after_collect);

    HAMMER_TRACE_GC(
        "Collection took {} ms. New heap size is {} ({} objects). Next "
        "auto-collect at heap size {}.",
        duration, size_after_collect, objects_after_collect, next_threshold_);
}

void Collector::trace_heap(Context& ctx) {
    to_trace_.clear();

    // Visit all root objects
    Walker w{this};
    ctx.walk(w);

    // Visit all reachable objects
    while (!to_trace_.empty()) {
        Value v = to_trace_.back();
        to_trace_.pop_back();
        trace(w, v);
    }
}

void Collector::sweep_heap(Context& ctx) {
    Heap& heap = ctx.heap();
    ObjectList& objects = heap.objects_;

    auto cursor = objects.cursor();
    while (cursor) {
        Header* hdr = cursor.get();
        if (!(hdr->flags_ & Header::FLAG_MARKED)) {
            cursor.remove();

            HAMMER_TRACE_GC(
                "Collecting object {}", to_string(Value::from_heap(hdr)));

            heap.destroy(hdr);
        } else {
            hdr->flags_ &= ~Header::FLAG_MARKED;
            cursor.next();
        }
    }
}

void Collector::mark(Value v) {
    if (v.is_null() || !v.is_heap_ptr())
        return;

    Header* object = v.heap_ptr();
    HAMMER_ASSERT(object, "Invalid heap pointer.");

    if (object->flags_ & Header::FLAG_MARKED) {
        return;
    }
    object->flags_ |= Header::FLAG_MARKED;

    // TODO: Visit the type of v as well once we have class objects.
    if (may_contain_references(v.type())) {
        to_trace_.push_back(v);
    }
}

void Collector::trace(Walker& w, Value v) {
    switch (v.type()) {
#define HAMMER_VM_TYPE(Name) \
    case ValueType::Name:    \
        (Name(v)).walk(w);   \
        break;

#include "hammer/vm/objects/types.inc"
    }
}

size_t Collector::compute_next_threshold(
    size_t last_threshold, size_t current_heap_size) {
    if (current_heap_size <= (last_threshold / 3) * 2) {
        return last_threshold;
    }

    if (current_heap_size > max_pow2<size_t>()) {
        return size_t(-1);
    }
    return ceil_pow2<size_t>(current_heap_size);
}

} // namespace hammer::vm
