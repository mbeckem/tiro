#include <catch.hpp>

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/coroutine.hpp"

using namespace hammer;
using namespace vm;

static_assert(std::is_trivially_copyable_v<Value>);
static_assert(std::is_trivially_copyable_v<UserFrame>);
static_assert(std::is_trivially_destructible_v<Value>);
static_assert(std::is_trivially_destructible_v<UserFrame>);

// alignment of Frame could be higher than value, then we would have to pad.
// It cannot be lower.
static_assert(alignof(CoroutineFrame) == alignof(Value));
static_assert(alignof(UserFrame) == alignof(Value));

TEST_CASE("Frame data is at offset 0", "[coroutine]") {
    Context ctx;

    Root tmpl(ctx, FunctionTemplate::make(ctx, {}, {}, 0, 0, {}));

    // TODO This should be done at compile time, but offsetof() does not support
    // base classes.

    auto base_class_offset = [](auto* object) {
        CoroutineFrame* frame = static_cast<CoroutineFrame*>(object);
        return reinterpret_cast<char*>(frame) - reinterpret_cast<char*>(object);
    };

    UserFrame user_frame(0, 0, nullptr, tmpl, {});
    REQUIRE(sizeof(UserFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&user_frame) == 0);

    AsyncFrame async_frame(0, 0, nullptr, NativeAsyncFunction());
    REQUIRE(sizeof(AsyncFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&async_frame) == 0);
}
