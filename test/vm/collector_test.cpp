#include <catch.hpp>

#include "tiro/vm/context.hpp"
#include "tiro/vm/heap/collector.hpp"
#include "tiro/vm/objects/arrays.hpp"
#include "tiro/vm/objects/strings.hpp"

#include "tiro/vm/context.ipp"
#include "tiro/vm/objects/arrays.ipp"
#include "tiro/vm/objects/classes.ipp"
#include "tiro/vm/objects/coroutines.ipp"
#include "tiro/vm/objects/functions.ipp"
#include "tiro/vm/objects/hash_tables.ipp"
#include "tiro/vm/objects/modules.ipp"
#include "tiro/vm/objects/native_objects.ipp"
#include "tiro/vm/objects/primitives.ipp"
#include "tiro/vm/objects/strings.ipp"
#include "tiro/vm/objects/tuples.ipp"

using namespace tiro;
using namespace tiro::vm;

// TODO: Heap/Collector/Context should be decoupled for easier testing

namespace {

// Tracks all encountered objects in a set.
struct TestWalker {
    std::unordered_set<uintptr_t> seen_;

    void clear() { seen_.clear(); }

    bool insert(const void* addr) {
        [[maybe_unused]] auto [pos, inserted] = seen_.insert(
            reinterpret_cast<uintptr_t>(addr));
        return inserted;
    }

    bool seen(uintptr_t addr) { return seen_.count(addr); }

    void walk_reachable(Value v) {
        switch (v.type()) {
#define TIRO_VM_TYPE(Name)   \
    case ValueType::Name:      \
        (Name(v)).walk(*this); \
        break;

#include "tiro/vm/objects/types.inc"
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
    void array(ArrayVisitor<T> array) {
        while (array.has_item()) {
            operator()(array.get_item());
            array.advance();
        }
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
