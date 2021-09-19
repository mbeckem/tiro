#include "vm/heap/new_collector.hpp"

#include "fmt/format.h"

#include "common/assert.hpp"
#include "common/scope_guards.hpp"
#include "common/type_traits.hpp"
#include "vm/heap/new_heap.hpp"
#include "vm/objects/all.hpp"
#include "vm/root_set.hpp"

#include "vm/root_set.ipp"

#if 1
#    define TIRO_TRACE_COLLECTOR(...) fmt::print("collector: " __VA_ARGS__);
#else
#    define TIRO_TRACE_COLLECTOR(...)
#endif

namespace tiro::vm::new_heap {

static size_t compute_next_threshold(size_t last_threshold, size_t current_heap_size);

template<typename TimePoint>
static double elapsed_ms(TimePoint start, TimePoint end);

std::string_view to_string(GcReason trigger) {
    switch (trigger) {
    case GcReason::Automatic:
        return "Automatic";
    case GcReason::Forced:
        return "Forced";
    case GcReason::AllocFailure:
        return "AllocFailure";
    }

    TIRO_UNREACHABLE("invalid gc reason");
}

Collector::Collector(Heap& heap)
    : heap_(heap) {}

Collector::~Collector() {}

void Collector::collect([[maybe_unused]] GcReason reason) {
    TIRO_DEBUG_ASSERT(!running_, "collector is already running");
    running_ = true;
    ScopeExit reset_running = [&]() { running_ = false; };

    [[maybe_unused]] const size_t size_before_collect = heap_.allocated_bytes();
    [[maybe_unused]] const size_t objects_before_collect = heap_.allocated_objects();
    TIRO_TRACE_COLLECTOR("Invoking collect() at heap size {} ({} objects). Reason: {}.",
        size_before_collect, objects_before_collect, to_string(reason));

    const auto start = std::chrono::steady_clock::now();
    {
        if (roots_)
            trace(*roots_);
        sweep(heap_);
    }
    const auto duration = last_duration_ = elapsed_ms(start, std::chrono::steady_clock::now());

    [[maybe_unused]] const size_t size_after_collect = heap_.allocated_bytes();
    [[maybe_unused]] const size_t objects_after_collect = heap_.allocated_objects();
    next_threshold_ = compute_next_threshold(next_threshold_, size_after_collect);

    TIRO_TRACE_COLLECTOR(
        "Collection took {} ms. New heap size is {} ({} objects). Next "
        "auto-collect at heap size {}.",
        duration, size_after_collect, objects_after_collect, next_threshold_);
}

class Collector::Tracer final {
public:
    Tracer(Collector& parent)
        : collector_(parent) {}

    Tracer(const Tracer&) = delete;
    Tracer& operator=(const Tracer&) = delete;

    /// Public entry point 1: Single value.
    void operator()(Value& value) { collector_.mark(value); }

    /// Public entry point 2: Special case for fat hash table entries.
    void operator()(HashTableEntry& value) {
        value.trace(*this); // calls back to operator()(Value&)
    }

    /// Public entry point 3: Array support.
    template<typename T>
    void operator()(Span<T> values) {
        // TODO: Could be optimized for large arrays by not pushing every item on the stack;
        //       push an array visitor instead.
        for (auto& v : values) {
            // could be either value or hash table entry
            operator()(v);
        }
    }

private:
    Collector& collector_;
};

void Collector::trace(RootSet& roots) {
    TIRO_DEBUG_ASSERT(running_, "must be running");
    TIRO_DEBUG_ASSERT(to_trace_.empty(), "trace stack must be empty");

    // Visit all root objects.
    // The tracer will call `mark(value)` for every value it encounters.
    Tracer tracer{*this};
    roots.trace(tracer);

    // Visit all reachable objects
    while (!to_trace_.empty()) {
        Value value = to_trace_.back();
        to_trace_.pop_back();
        trace_value(value, tracer);
    }
}

void Collector::mark(Value value) {
    TIRO_DEBUG_ASSERT(running_, "must be running");

    if (value.is_null() || !value.is_heap_ptr())
        return;

    Header* header = static_cast<HeapValue>(value).heap_ptr();
    TIRO_DEBUG_ASSERT(header, "Invalid heap pointer.");

    if (header->large_object()) {
        auto lob = LargeObject::from_address(header);
        if (lob->is_marked())
            return;

        lob->set_marked(true);
    } else {
        auto page = Page::from_address(header, heap_);
        auto index = page->cell_index(header);
        if (page->is_cell_marked(index))
            return;

        page->set_cell_marked(index, true);
    }

    to_trace_.push_back(value);
}

void Collector::trace_value(Value value, Tracer& tracer) {
    const auto trace_impl = [this, &tracer](auto concrete_value) {
        using ConcreteValueType = remove_cvref_t<decltype(concrete_value)>;

        // Layout code is only valid for actual heap types.
        if constexpr (std::is_base_of_v<HeapValue, ConcreteValueType>) {
            using Layout = typename ConcreteValueType::Layout;
            using Traits = LayoutTraits<Layout>;

            if constexpr (Traits::may_contain_references) {
                // Visit the type instance of the current value, which is important for user defined types.
                // It is fine to skip this if `may_contain_references` is false, because
                // only some builtin types do not contain references and those types are visited anyway
                // through `TypeSystem::trace()`.
                // NOTE: This also means that builtin type instances that represent objects without references (e.g. String type)
                //       may never move.
                // TODO: Do 'may_contain_references' on a page level before visiting the object itself?
                mark(HeapValue(concrete_value.heap_ptr()->type()));

                const auto layout = concrete_value.layout();
                TIRO_DEBUG_ASSERT(layout != nullptr, "pointer to heap value must not be null");
                Traits::trace(layout, tracer); // ends up invoking mark again for visited values
            }
        } else {
            (void) concrete_value;
        }
    };

    switch (value.type()) {
#define TIRO_CASE(ConcreteValueType)                       \
    case TypeToTag<ConcreteValueType>:                     \
        trace_impl(static_cast<ConcreteValueType>(value)); \
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

void Collector::sweep(Heap& heap) {
    heap.sweep();
}

static size_t compute_next_threshold(size_t last_threshold, size_t current_heap_size) {
    if (current_heap_size <= (last_threshold / 3) * 2) {
        return last_threshold;
    }

    if (current_heap_size > max_pow2<size_t>()) {
        return size_t(-1);
    }
    return ceil_pow2_fast<size_t>(current_heap_size);
}

template<typename TimePoint>
static double elapsed_ms(TimePoint start, TimePoint end) {
    std::chrono::duration<double, std::milli> millis = end - start;
    return millis.count();
}

} // namespace tiro::vm::new_heap
