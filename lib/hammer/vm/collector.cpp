#include "hammer/vm/collector.hpp"

#include "hammer/core/span.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/heap.hpp"
#include "hammer/vm/object.hpp"

#include "hammer/vm/context.ipp"
#include "hammer/vm/object.ipp"

namespace hammer::vm {

struct Collector::Walker {
    Collector* gc;

    void operator()(Value& v) { gc->mark(v); }

    void operator()(Span<Value> span) {
        for (Value& v : span)
            gc->mark(v);
    }
};

Collector::Collector() {}

void Collector::collect(Context& ctx) {
    // Mark (trace) phase
    {
        Walker w{this};
        stack_.clear();

        // Visit all root objects
        ctx.walk(w);

        // Visit all reachable objects
        while (!stack_.empty()) {
            Value v = stack_.back();
            stack_.pop_back();
            trace(w, v);
        }
    }

    // Sweep phase
    {
        Heap& heap = ctx.heap();
        ObjectList& objects = heap.objects_;
        auto cursor = objects.cursor();
        while (cursor) {
            Header* hdr = cursor.get();
            if (!(hdr->flags_ & Header::FLAG_MARKED)) {
                cursor.remove();
                heap.free(hdr);
            } else {
                hdr->flags_ &= ~Header::FLAG_MARKED;
                cursor.next();
            }
        }
    }
}

void Collector::mark(Value v) {
    if (v.is_null() || !v.is_heap_ptr())
        return;

    Header* object = v.heap_ptr();
    HAMMER_ASSERT(object, "Invalid heap pointer.");

    if (object->flags_ & Header::FLAG_MARKED)
        return;

    object->flags_ |= Header::FLAG_MARKED;
    stack_.push_back(v);
}

void Collector::trace(Walker& w, Value v) {
    switch (v.type()) {
#define WALK_HEAP_TYPE(Name) \
    case ValueType::Name:    \
        (Name(v)).walk(w);   \
        break;

        HAMMER_HEAP_TYPES(WALK_HEAP_TYPE)

#undef WALK_HEAP_TYPE
    }
}

} // namespace hammer::vm
