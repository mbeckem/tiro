#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/objects/coroutine_stack.hpp"

namespace tiro::vm::test {

static_assert(std::is_trivially_copyable_v<Value>);
static_assert(std::is_trivially_destructible_v<Value>);

static_assert(std::is_trivially_copyable_v<SyncFrame>);
static_assert(std::is_trivially_copyable_v<CodeFrame>);
static_assert(std::is_trivially_copyable_v<CatchFrame>);
static_assert(std::is_trivially_copyable_v<ResumableFrame>);
static_assert(std::is_trivially_destructible_v<SyncFrame>);
static_assert(std::is_trivially_destructible_v<CodeFrame>);
static_assert(std::is_trivially_destructible_v<CatchFrame>);
static_assert(std::is_trivially_destructible_v<ResumableFrame>);

// Alignment of Frame could be higher than value, then we would have to pad.
// It must not be lower.
static_assert(alignof(CoroutineFrame) == alignof(Value));
static_assert(alignof(SyncFrame) == alignof(Value));
static_assert(alignof(CodeFrame) == alignof(Value));
static_assert(alignof(CatchFrame) == alignof(Value));
static_assert(alignof(ResumableFrame) == alignof(Value));

TEST_CASE("Function frames should have the correct layout", "[coroutine]") {
    Context ctx;

    Scope sc(ctx);
    Local name = sc.local(String::make(ctx, "Test"));
    Local members = sc.local(Tuple::make(ctx, 0));
    Local exported = sc.local(HashTable::make(ctx));
    Local module = sc.local(Module::make(ctx, name, members, exported));
    Local tmpl = sc.local(CodeFunctionTemplate::make(ctx, name, module, 0, 0, {}, {}));

    auto base_class_offset = [](auto* object) {
        CoroutineFrame* frame = static_cast<CoroutineFrame*>(object);
        return reinterpret_cast<char*>(frame) - reinterpret_cast<char*>(object);
    };

    CodeFrame user_frame(0, 0, nullptr, *tmpl, {});
    REQUIRE(sizeof(CodeFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&user_frame) == 0);

    Local sync_func = sc.local(NativeFunction::make(
        ctx, name, {}, 0, 0, NativeFunctionStorage::sync([](SyncFrameContext&) {})));
    SyncFrame sync_frame(0, 0, nullptr, *sync_func);
    REQUIRE(sizeof(SyncFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&sync_frame) == 0);

    Local async_func = sc.local(NativeFunction::make(
        ctx, name, {}, 0, 0, NativeFunctionStorage::async([](AsyncFrameContext) {})));
    AsyncFrame async_frame(0, 0, nullptr, *async_func);
    REQUIRE(sizeof(AsyncFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&async_frame) == 0);

    CatchFrame catch_frame(0, 0, nullptr);
    REQUIRE(sizeof(CatchFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&catch_frame) == 0);

    Local resumable_func = sc.local(NativeFunction::make(
        ctx, name, {}, 0, 0, NativeFunctionStorage::resumable([](ResumableFrameContext&) {})));
    ResumableFrame resumable_frame(0, 0, nullptr, *resumable_func);
    REQUIRE(sizeof(ResumableFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&resumable_frame) == 0);
}

} // namespace tiro::vm::test
