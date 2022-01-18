#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/objects/coroutine_stack.hpp"

namespace tiro::vm::test {

static_assert(std::is_trivially_copyable_v<Value>);
static_assert(std::is_trivially_destructible_v<Value>);

static_assert(std::is_trivially_copyable_v<CodeFrame>);
static_assert(std::is_trivially_copyable_v<CatchFrame>);
static_assert(std::is_trivially_copyable_v<ResumableFrame>);
static_assert(std::is_trivially_destructible_v<CodeFrame>);
static_assert(std::is_trivially_destructible_v<CatchFrame>);
static_assert(std::is_trivially_destructible_v<ResumableFrame>);

// Alignment of Frame could be higher than value, then we would have to pad.
// It must not be lower.
static_assert(alignof(CoroutineFrame) == alignof(Value));
static_assert(alignof(CodeFrame) == alignof(Value));
static_assert(alignof(CatchFrame) == alignof(Value));
static_assert(alignof(ResumableFrame) == alignof(Value));

TEST_CASE("Function frames should have the correct layout", "[coroutine-stack]") {
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

    CodeFrame user_frame(*tmpl, {}, CoroutineFrameParams{});
    REQUIRE(sizeof(CodeFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&user_frame) == 0);

    Local async_func = sc.local(NativeFunction::make(
        ctx, name, {}, 0, 0, NativeFunctionStorage::async([](AsyncFrameContext) {})));
    AsyncFrame async_frame(*async_func, CoroutineFrameParams{});
    REQUIRE(sizeof(AsyncFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&async_frame) == 0);

    CatchFrame catch_frame(CoroutineFrameParams{});
    REQUIRE(sizeof(CatchFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&catch_frame) == 0);

    Local resumable_func = sc.local(NativeFunction::make(
        ctx, name, {}, 0, 0, NativeFunctionStorage::resumable([](ResumableFrameContext&) {})));
    ResumableFrame resumable_frame(*resumable_func, CoroutineFrameParams{});
    REQUIRE(sizeof(ResumableFrame) % sizeof(Value) == 0);
    REQUIRE(base_class_offset(&resumable_frame) == 0);
}

TEST_CASE("Function frames compute their caller correctly", "[coroutine-stack]") {
    Context ctx;

    Scope sc(ctx);
    Local name = sc.local(String::make(ctx, "Test"));
    Local async_func = sc.local(NativeFunction::make(
        ctx, name, {}, 0, 0, NativeFunctionStorage::async([](AsyncFrameContext) {})));

    alignas(Value) byte stack[1 << 10];
    byte* end = stack + (1 << 10);
    byte* top = &stack[0];
    auto alloc = [&](u32 n) {
        if (end - top < n)
            throw std::bad_alloc();

        byte* ret = top;
        top += n;
        return ret;
    };

    // Caller frame on top
    AsyncFrame* caller = new (alloc(sizeof(AsyncFrame)))
        AsyncFrame(*async_func, CoroutineFrameParams());
    REQUIRE(caller->caller_offset == 0);
    REQUIRE(caller->caller() == nullptr);

    // Create a trivial gap
    alloc(sizeof(Value));

    // Callee frame after gap
    CoroutineFrameParams callee_params;
    callee_params.caller = caller;
    AsyncFrame* callee = new (alloc(sizeof(AsyncFrame))) AsyncFrame(*async_func, callee_params);

    REQUIRE(callee->caller_offset == sizeof(AsyncFrame) + sizeof(Value));
    REQUIRE(callee->caller() == caller);
};

} // namespace tiro::vm::test
