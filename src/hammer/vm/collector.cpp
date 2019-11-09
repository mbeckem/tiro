#include "hammer/vm/collector.hpp"

#include "hammer/core/span.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/heap.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/raw_arrays.hpp"

#include "hammer/vm/context.ipp"
#include "hammer/vm/objects/array.ipp"
#include "hammer/vm/objects/coroutine.ipp"
#include "hammer/vm/objects/function.ipp"
#include "hammer/vm/objects/hash_table.ipp"
#include "hammer/vm/objects/object.ipp"
#include "hammer/vm/objects/string.ipp"

namespace hammer::vm {

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

void Collector::collect(Context& ctx) {
    // Mark (trace) phase
    {
        Walker w{this};
        to_trace_.clear();

        // Visit all root objects
        ctx.walk(w);

        // Visit all reachable objects
        while (!to_trace_.empty()) {
            Value v = to_trace_.back();
            to_trace_.pop_back();
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
                heap.destroy(hdr);
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

    if (may_contain_references(v.type())) {
        to_trace_.push_back(v);
    }
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
