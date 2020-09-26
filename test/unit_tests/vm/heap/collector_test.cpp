#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/heap/collector.hpp"
#include "vm/objects/all.hpp"

#include "vm/context.ipp"

using namespace tiro;
using namespace tiro::vm;

// TODO: Heap/Collector/Context should be decoupled for easier testing

namespace {

// Tracks all encountered objects in a set.
struct TestTracer {
public:
    void clear() {
        seen_slots_.clear();
        seen_values_.clear();
    }

    bool seen_slot(uintptr_t addr) { return seen_slots_.count(addr); }

    u32 value_count(Value v) {
        auto pos = seen_values_.find(v.raw());
        return pos == seen_values_.end() ? 0 : pos->second;
    }

    void operator()(Value& v) {
        insert_value(v);
        if (insert_slot(&v)) {
            dispatch(v);
        }
    }

    void operator()(HashTableEntry& e) {
        insert_value(e.key());
        insert_value(e.value());
        if (insert_slot(&e)) {
            dispatch(e.key());
            dispatch(e.value());
        }
    }

    template<typename T>
    void operator()(Span<T> span) {
        for (auto& v : span)
            operator()(v);
    }

    void dispatch(Value v) {
        switch (v.type()) {
#define TIRO_CASE(Type)     \
    case TypeToTag<Type>:   \
        walk_impl(Type(v)); \
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
            TIRO_CASE(Coroutine)
            TIRO_CASE(CoroutineStack)
            TIRO_CASE(CoroutineToken)
            TIRO_CASE(Environment)
            TIRO_CASE(Float)
            TIRO_CASE(Function)
            TIRO_CASE(FunctionTemplate)
            TIRO_CASE(HashTable)
            TIRO_CASE(HashTableIterator)
            TIRO_CASE(HashTableKeyIterator)
            TIRO_CASE(HashTableKeyView)
            TIRO_CASE(HashTableStorage)
            TIRO_CASE(HashTableValueIterator)
            TIRO_CASE(HashTableValueView)
            TIRO_CASE(Integer)
            TIRO_CASE(InternalType)
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
            // [[[end]]]
        }
    }

private:
    template<typename ValueT>
    void walk_impl(ValueT v) {
        if constexpr (std::is_base_of_v<HeapValue, ValueT>) {
            using Layout = typename ValueT::Layout;
            using Traits = LayoutTraits<Layout>;
            if constexpr (Traits::may_contain_references) {
                Traits::trace(v.layout(), *this);
            }
        }
        (void) v;
    }

    bool insert_slot(const void* addr) {
        [[maybe_unused]] auto [pos, inserted] = seen_slots_.insert(
            reinterpret_cast<uintptr_t>(addr));
        return inserted;
    }

    void insert_value(Value value) { ++seen_values_[value.raw()]; }

private:
    std::unordered_set<uintptr_t> seen_slots_;
    std::unordered_map<uintptr_t, u32> seen_values_;
};

} // namespace

TEST_CASE("Collector should collect unreferenced objects", "[collector]") {
    Context ctx;

    Heap& heap = ctx.heap();
    Collector& gc = heap.collector();
    gc.collect(ctx, GcTrigger::Forced);

    const size_t allocated_objects_before = heap.allocated_objects();
    const size_t allocated_bytes_before = heap.allocated_bytes();

    auto allocated_objects = [&] {
        const auto alloc = heap.allocated_objects();
        REQUIRE(alloc >= allocated_objects_before);
        return alloc - allocated_objects_before;
    };

    auto allocated_bytes = [&] {
        const auto alloc = heap.allocated_bytes();
        REQUIRE(alloc >= allocated_bytes_before);
        return alloc - allocated_bytes_before;
    };

    REQUIRE(allocated_objects() == 0);
    REQUIRE(allocated_bytes() == 0);

    {
        Scope sc1(ctx);

        Local v1 = sc1.local<Value>(Integer::make(ctx, 123));
        Local v2 = sc1.local<Value>(Array::make(ctx, 1024));
        Local v3 = sc1.local<Value>(String::make(ctx, "Hello World"));

        {
            Scope sc2(ctx);
            Local add = sc2.local(String::make(ctx, "Array member"));
            v2.must_cast<Array>()->append(ctx, add);
            v2.must_cast<Array>()->append(ctx, v3);
        }

        // +1: ArrayStorage created by array
        REQUIRE(allocated_objects() == 5);
        REQUIRE(allocated_bytes() > 0);

        // This collection is a no-op
        gc.collect(ctx, GcTrigger::Forced);
        REQUIRE(allocated_objects() == 5);
        REQUIRE(allocated_bytes() > 0);

        // Integer is released, but string is still referenced from the array
        v1.set(Value::null());
        v3.set(Value::null());
        gc.collect(ctx, GcTrigger::Forced);
        REQUIRE(allocated_objects() == 4);
        REQUIRE(allocated_bytes() > 0);
    }

    // All roots in this function have been released
    gc.collect(ctx, GcTrigger::Forced);
    REQUIRE(allocated_objects() == 0);
    REQUIRE(allocated_bytes() == 0);
}

TEST_CASE("Collector should find rooted local objects", "[collector]") {
    Context ctx;

    Scope sc(ctx);
    Local value = sc.local();

    TestTracer walker;
    ctx.trace(walker);
    REQUIRE(walker.seen_slot(reinterpret_cast<uintptr_t>(get_valid_slot(value))));
}

TEST_CASE("Collector should find external values", "[collector]") {
    Context ctx;
    auto& storage = ctx.externals();

    External used_handle = storage.allocate(String::make(ctx, "Hello"));
    auto used_slot = get_valid_slot(used_handle);

    External free_handle = storage.allocate();
    auto free_slot = get_valid_slot(free_handle);
    storage.free(free_handle);

    TestTracer walker;
    ctx.trace(walker);

    REQUIRE(walker.seen_slot(reinterpret_cast<uintptr_t>(used_slot)));
    REQUIRE(!walker.seen_slot(reinterpret_cast<uintptr_t>(free_slot)));
}

// TODO: More complex test cases for reachablity, for example
// values in nested data structures, only reachable through the call stack etc..
