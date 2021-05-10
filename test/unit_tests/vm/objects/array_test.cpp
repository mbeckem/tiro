#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/math.hpp"
#include "vm/objects/array.hpp"

#include "support/vm_matchers.hpp"

namespace tiro::vm::test {

using test_support::is_integer_value;

TEST_CASE("Arrays should support insertion", "[arrays]") {
    Context ctx;

    Scope sc(ctx);
    Local array = sc.local(Array::make(ctx, 0));

    {
        Local integer = sc.local();
        for (i64 i = 0; i < 5000; ++i) {
            integer.set(HeapInteger::make(ctx, i));
            array->append(ctx, integer);
        }
    }

    REQUIRE(array->size() == 5000);
    REQUIRE(array->capacity() == 8192);
    for (size_t i = 0; i < 5000; ++i) {
        Value value = array->get(i);
        if (!value.is<HeapInteger>()) {
            CAPTURE(to_string(value.type()));
            FAIL("Expected an integer");
        }

        HeapInteger integer(value);

        if (integer.value() != i64(i)) {
            CAPTURE(i, integer.value());
            FAIL("Unexpected value");
        }
    }
}

TEST_CASE("Arrays should support clearing", "[arrays]") {
    Context ctx;
    Scope sc(ctx);
    Local array = sc.local(Array::make(ctx, 0));

    {
        Local value = sc.local();
        for (int i = 0; i < 19; ++i) {
            value = ctx.get_integer(i);
            array->append(ctx, value);
        }
    }
    REQUIRE(array->size() == 19);
    REQUIRE(array->capacity() == 32);

    array->clear();
    REQUIRE(array->size() == 0);
    REQUIRE(array->capacity() == 32);

    Local value = sc.local(ctx.get_integer(123));
    array->append(ctx, value);
    REQUIRE(array->size() == 1);
    REQUIRE_THAT(array->get(0), is_integer_value(123));
}

} // namespace tiro::vm::test
