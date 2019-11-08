#include <catch.hpp>

#include "hammer/vm/objects/array.hpp"
#include "hammer/vm/objects/raw_arrays.hpp"

using namespace hammer;
using namespace hammer::vm;

TEST_CASE("Raw arrays work", "[arrays]") {
    Context ctx;

    const size_t size = 1 << 16;

    Root<U16Array> array(ctx, U16Array::make(ctx, size, 7));
    REQUIRE(!array->is_null());
    REQUIRE(array->size() == size);
    REQUIRE(array->data() != nullptr);

    auto values = array->values();
    REQUIRE(values.size() == size);
    for (size_t i = 0; i < size; ++i) {
        if (values[i] != 7) {
            CAPTURE(i, values[i]);
            FAIL("Invalid value.");
        }
    }

    array->values()[477] = 488;
    REQUIRE(array->values()[477] == 488);
}

TEST_CASE("Insert values into an array", "[arrays]") {
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
