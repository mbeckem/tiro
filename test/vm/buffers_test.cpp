#include <catch.hpp>

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/buffers.hpp"

using namespace hammer;
using namespace hammer::vm;

TEST_CASE("Raw buffers should be able to store bytes", "[arrays]") {
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
