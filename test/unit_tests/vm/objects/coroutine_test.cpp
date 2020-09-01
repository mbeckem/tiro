#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/all.hpp"
#include "vm/objects/coroutine.hpp"

using namespace tiro;
using namespace vm;

static_assert(std::is_trivially_copyable_v<Value>);
static_assert(std::is_trivially_copyable_v<UserFrame>);
static_assert(std::is_trivially_destructible_v<Value>);
static_assert(std::is_trivially_destructible_v<UserFrame>);

// alignment of Frame could be higher than value, then we would have to pad.
// It cannot be lower.
static_assert(alignof(CoroutineFrame) == alignof(Value));
static_assert(alignof(SyncFrame) == alignof(Value));
static_assert(alignof(UserFrame) == alignof(Value));

TEST_CASE("Function frames should have the correct layout", "[coroutine]") {
    Context ctx;

    Scope sc(ctx);
    Local name = sc.local(String::make(ctx, "Test"));
    Local members = sc.local(Tuple::make(ctx, 0));
    Local exported = sc.local(HashTable::make(ctx));
    Local module = sc.local(Module::make(ctx, name, members, exported));
    Local tmpl = sc.local(FunctionTemplate::make(ctx, name, module, 0, 0, {}));

    auto base_class_offset = [](auto* object) {
        CoroutineFrame* frame = static_cast<CoroutineFrame*>(object);
        return reinterpret_cast<char*>(frame) - reinterpret_cast<char*>(object);
    };

    UserFrame user_frame(0, 0, nullptr, *tmpl, {});
    REQUIRE(sizeof(UserFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&user_frame) == 0);

    Local sync_func = sc.local(NativeFunction::make(
        ctx, name, {}, 0, NativeFunctionArg::sync([](NativeFunctionFrame&) {})));
    SyncFrame sync_frame(0, 0, nullptr, *sync_func);
    REQUIRE(sizeof(SyncFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&sync_frame) == 0);

    Local async_func = sc.local(NativeFunction::make(
        ctx, name, {}, 0, NativeFunctionArg::async([](NativeAsyncFunctionFrame) {})));
    AsyncFrame async_frame(0, 0, nullptr, *async_func);
    REQUIRE(sizeof(AsyncFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&async_frame) == 0);
}
