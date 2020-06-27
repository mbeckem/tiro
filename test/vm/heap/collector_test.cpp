#include <catch.hpp>

#include "vm/context.hpp"
#include "vm/heap/collector.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/buffer.hpp"
#include "vm/objects/module.hpp"
#include "vm/objects/native_object.hpp"
#include "vm/objects/string.hpp"

#include "vm/context.ipp"

using namespace tiro;
using namespace tiro::vm;

// TODO: Heap/Collector/Context should be decoupled for easier testing

namespace {

// Tracks all encountered objects in a set.
struct TestWalker {
    std::unordered_set<uintptr_t> seen_;

    void clear() { seen_.clear(); }

    bool insert(const void* addr) {
        [[maybe_unused]] auto [pos, inserted] = seen_.insert(reinterpret_cast<uintptr_t>(addr));
        return inserted;
    }

    bool seen(uintptr_t addr) { return seen_.count(addr); }

    void walk_reachable(Value v) {
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
            TIRO_CASE(ArrayStorage)
            TIRO_CASE(Boolean)
            TIRO_CASE(BoundMethod)
            TIRO_CASE(Buffer)
            TIRO_CASE(Code)
            TIRO_CASE(Coroutine)
            TIRO_CASE(CoroutineStack)
            TIRO_CASE(DynamicObject)
            TIRO_CASE(Environment)
            TIRO_CASE(Float)
            TIRO_CASE(Function)
            TIRO_CASE(FunctionTemplate)
            TIRO_CASE(HashTable)
            TIRO_CASE(HashTableIterator)
            TIRO_CASE(HashTableStorage)
            TIRO_CASE(Integer)
            TIRO_CASE(Method)
            TIRO_CASE(Module)
            TIRO_CASE(NativeAsyncFunction)
            TIRO_CASE(NativeFunction)
            TIRO_CASE(NativeObject)
            TIRO_CASE(NativePointer)
            TIRO_CASE(Null)
            TIRO_CASE(SmallInteger)
            TIRO_CASE(String)
            TIRO_CASE(StringBuilder)
            TIRO_CASE(Symbol)
            TIRO_CASE(Tuple)
            TIRO_CASE(Type)
            TIRO_CASE(Undefined)
            // [[[end]]]
        }
    }

    void operator()(Value& v) {
        if (insert(&v)) {
            walk_reachable(v);
        }
    }

    void operator()(HashTableEntry& e) {
        if (insert(&e)) {
            walk_reachable(e.key());
            walk_reachable(e.value());
        }
    }

    template<typename T>
    void operator()(Span<T> span) {
        for (auto& v : span)
            operator()(v);
    }

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
        Root<Value> v1(ctx, Integer::make(ctx, 123));
        Root v2(ctx, Array::make(ctx, 1024));
        Root<Value> v3(ctx, String::make(ctx, "Hello World"));

        {
            Root add(ctx, String::make(ctx, "Array member"));
            v2->append(ctx, add.handle());
            v2->append(ctx, v3.handle());
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

TEST_CASE("Collector should find rooted objects", "[collector]") {
    Context ctx;

    Root<Value> value(ctx);

    TestWalker walker;
    ctx.walk(walker);
    REQUIRE(walker.seen(value.slot_address()));
}

TEST_CASE("Collector should find global objects", "[collector]") {
    Context ctx;

    Global<Value> value(ctx);

    TestWalker walker;
    ctx.walk(walker);
    REQUIRE(walker.seen(value.slot_address()));
}

// TODO: More complex test cases for reachablity, for example
// values in nested data structures, only reachable through the call stack etc..
