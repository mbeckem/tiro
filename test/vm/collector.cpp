#include <catch.hpp>

#include "hammer/vm/collector.hpp"
#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/array.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/string.hpp"

using namespace hammer;
using namespace hammer::vm;

// TODO: Heap/Collector/Context should be decoupled for easier testing

TEST_CASE("collects unreferenced objects", "[collector]") {
    Context ctx;

    Heap& heap = ctx.heap();
    Collector& gc = ctx.collector();

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
        gc.collect(ctx);
        REQUIRE(allocated_objects() == 5);
        REQUIRE(allocated_bytes() > 0);

        // Integer is released, but string is still referenced from the array
        v1.set(Value::null());
        v3.set(Value::null());
        gc.collect(ctx);
        REQUIRE(allocated_objects() == 4);
        REQUIRE(allocated_bytes() > 0);
    }

    // All roots in this function have been released
    gc.collect(ctx);
    REQUIRE(allocated_objects() == 0);
    REQUIRE(allocated_bytes() == 0);
}
