#include "vm/heap/collector.hpp"

#include "common/adt/span.hpp"
#include "vm/context.hpp"
#include "vm/heap/heap.hpp"
#include "vm/objects/all.hpp"

#include "vm/root_set.ipp"

#include <chrono>

// Enable debug logging
// #define TIRO_TRACE_GC_ENABLED

#ifdef TIRO_TRACE_GC_ENABLED
#    include <iostream>
#    define TIRO_TRACE_GC(...) (std::cout << "Collector: " << fmt::format(__VA_ARGS__) << std::endl)
#else
#    define TIRO_TRACE_GC(...)
#endif

namespace tiro::vm {

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

    TIRO_UNREACHABLE("Invalid trigger value.");
}

struct Collector::Tracer {
    Collector* gc;

    void operator()(Value& v) { gc->mark(v); }

    void operator()(HashTableEntry& e) { e.trace(*this); }

    template<typename T>
    void operator()(Span<T> span) {
        // TODO dont visit all members of an array at once, instead
        // push the visitor itself on the stack.
        for (auto& v : span) {
            operator()(v);
        }
    }
};

Collector::Collector() {}

void Collector::collect(Context& ctx, [[maybe_unused]] GcTrigger trigger) {
    TIRO_DEBUG_ASSERT(
        this == &ctx.heap().collector(), "Collector does not belong to this context.");

    [[maybe_unused]] const size_t size_before_collect = ctx.heap().allocated_bytes();
    [[maybe_unused]] const size_t objects_before_collect = ctx.heap().allocated_objects();

    TIRO_TRACE_GC("Invoking collect() at heap size {} ({} objects). Trigger: {}.",
        size_before_collect, objects_before_collect, to_string(trigger));

    const auto start = std::chrono::steady_clock::now();
    {
        trace_heap(ctx);
        sweep_heap(ctx);
    }
    const auto end = std::chrono::steady_clock::now();
    const auto duration = elapsed_ms(start, end);

    const size_t size_after_collect = ctx.heap().allocated_bytes();
    [[maybe_unused]] const size_t objects_after_collect = ctx.heap().allocated_objects();

    last_duration_ = duration;
    next_threshold_ = compute_next_threshold(next_threshold_, size_after_collect);

    TIRO_TRACE_GC(
        "Collection took {} ms. New heap size is {} ({} objects). Next "
        "auto-collect at heap size {}.",
        duration, size_after_collect, objects_after_collect, next_threshold_);
}

void Collector::trace_heap(Context& ctx) {
    to_trace_.clear();

    // Visit all root objects
    Tracer t{this};
    ctx.trace(t);

    // Visit all reachable objects
    while (!to_trace_.empty()) {
        Value v = to_trace_.back();
        to_trace_.pop_back();
        trace(v, t);
    }
}

void Collector::sweep_heap(Context& ctx) {
    Heap& heap = ctx.heap();
    ObjectList& objects = heap.objects_;

    auto cursor = objects.cursor();
    while (cursor) {
        Header* hdr = cursor.get();
        if (!(hdr->marked())) {
            cursor.remove();

            TIRO_TRACE_GC("Collecting object {}", to_string(HeapValue(hdr)));

            heap.destroy(hdr);
        } else {
            hdr->marked(false);
            cursor.next();
        }
    }
}

void Collector::mark(Value v) {
    if (v.is_null() || !v.is_heap_ptr())
        return;

    Header* object = HeapValue(v).heap_ptr();
    TIRO_DEBUG_ASSERT(object, "Invalid heap pointer.");

    if (object->marked()) {
        return;
    }
    object->marked(true);

    // TODO: Layout information should be accessible through the type.
    mark(HeapValue(object->type()));
    if (may_contain_references(v.type())) {
        to_trace_.push_back(v);
    }
}

void Collector::trace(Value v, Tracer& t) {
    switch (v.type()) {
#define TIRO_CASE(Type)         \
    case TypeToTag<Type>:       \
        trace_impl(Type(v), t); \
        break;

        /* [[[cog
            from cog import outl
            from codegen.objects import VM_OBJECTS
            for object in VM_OBJECTS:
                outl(f"TIRO_CASE({object.type_name})")
        ]]] */
        TIRO_CASE(Array)
        TIRO_CASE(ArrayIterator)
        TIRO_CASE(ArrayStorage)
        TIRO_CASE(Boolean)
        TIRO_CASE(BoundMethod)
        TIRO_CASE(Buffer)
        TIRO_CASE(Code)
        TIRO_CASE(CodeFunction)
        TIRO_CASE(CodeFunctionTemplate)
        TIRO_CASE(Coroutine)
        TIRO_CASE(CoroutineStack)
        TIRO_CASE(CoroutineToken)
        TIRO_CASE(Environment)
        TIRO_CASE(Exception)
        TIRO_CASE(Float)
        TIRO_CASE(HandlerTable)
        TIRO_CASE(HashTable)
        TIRO_CASE(HashTableIterator)
        TIRO_CASE(HashTableKeyIterator)
        TIRO_CASE(HashTableKeyView)
        TIRO_CASE(HashTableStorage)
        TIRO_CASE(HashTableValueIterator)
        TIRO_CASE(HashTableValueView)
        TIRO_CASE(HeapInteger)
        TIRO_CASE(InternalType)
        TIRO_CASE(MagicFunction)
        TIRO_CASE(Method)
        TIRO_CASE(Module)
        TIRO_CASE(NativeFunction)
        TIRO_CASE(NativeObject)
        TIRO_CASE(NativePointer)
        TIRO_CASE(Null)
        TIRO_CASE(Record)
        TIRO_CASE(RecordTemplate)
        TIRO_CASE(Result)
        TIRO_CASE(Set)
        TIRO_CASE(SetIterator)
        TIRO_CASE(SmallInteger)
        TIRO_CASE(String)
        TIRO_CASE(StringBuilder)
        TIRO_CASE(StringIterator)
        TIRO_CASE(StringSlice)
        TIRO_CASE(Symbol)
        TIRO_CASE(Tuple)
        TIRO_CASE(TupleIterator)
        TIRO_CASE(Type)
        TIRO_CASE(Undefined)
        TIRO_CASE(UnresolvedImport)
        // [[[end]]]

#undef TIRO_CASE
    }
}

template<typename ValueT>
void Collector::trace_impl(ValueT v, Tracer& t) {
    if constexpr (std::is_base_of_v<HeapValue, ValueT>) {
        using Layout = typename ValueT::Layout;
        using Traits = LayoutTraits<Layout>;

        if constexpr (Traits::may_contain_references) {
            const auto self = v.layout();
            TIRO_DEBUG_ASSERT(self != nullptr, "Pointer to heap value must not be null.");
            Traits::trace(self, t);
        }
    }

    (void) v;
    (void) t;
}

size_t Collector::compute_next_threshold(size_t last_threshold, size_t current_heap_size) {
    if (current_heap_size <= (last_threshold / 3) * 2) {
        return last_threshold;
    }

    if (current_heap_size > max_pow2<size_t>()) {
        return size_t(-1);
    }
    return ceil_pow2_fast<size_t>(current_heap_size);
}

} // namespace tiro::vm
