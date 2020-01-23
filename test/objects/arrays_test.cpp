#include <catch.hpp>

#include "tiro/objects/arrays.hpp"
#include "tiro/vm/context.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Arrays should support insertion", "[arrays]") {
    Context ctx;

    Root<Array> array(ctx, Array::make(ctx, 0));
    {
        Root<Integer> integer(ctx);
        for (i64 i = 0; i < 5000; ++i) {
            integer.set(Integer::make(ctx, i));
            array->append(ctx, integer.handle());
        }
    }

    REQUIRE(array->size() == 5000);
    REQUIRE(array->capacity() == 8192);
    for (size_t i = 0; i < 5000; ++i) {
        Value value = array->get(i);
        if (!value.is<Integer>()) {
            CAPTURE(to_string(value.type()));
            FAIL("Expected an integer");
        }

        Integer integer(value);

        if (integer.value() != i64(i)) {
            CAPTURE(i, integer.value());
            FAIL("Unexpected value");
        }
    }
}
