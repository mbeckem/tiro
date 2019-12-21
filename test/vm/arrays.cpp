#include <catch.hpp>

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/arrays.hpp"
#include "hammer/vm/objects/buffers.hpp"

using namespace hammer;
using namespace hammer::vm;

TEST_CASE("Raw buffers work", "[arrays]") {
    Context ctx;

    const size_t size = 1 << 16;

    Root<Buffer> buffer(ctx, Buffer::make(ctx, size, 7));
    REQUIRE(!buffer->is_null());
    REQUIRE(buffer->size() == size);
    REQUIRE(buffer->data() != nullptr);

    auto values = buffer->values();
    REQUIRE(values.size() == size);
    for (size_t i = 0; i < size; ++i) {
        if (values[i] != 7) {
            CAPTURE(i, values[i]);
            FAIL("Invalid value.");
        }
    }

    buffer->values()[477] = 123;
    REQUIRE(buffer->values()[477] == 123);
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
