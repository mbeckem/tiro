#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/all.hpp"
#include "vm/objects/coroutine.hpp"

namespace tiro::vm::test {

static_assert(std::is_trivially_copyable_v<Value>);
static_assert(std::is_trivially_copyable_v<CodeFrame>);
static_assert(std::is_trivially_destructible_v<Value>);
static_assert(std::is_trivially_destructible_v<CodeFrame>);

// alignment of Frame could be higher than value, then we would have to pad.
// It cannot be lower.
static_assert(alignof(CoroutineFrame) == alignof(Value));
static_assert(alignof(SyncFrame) == alignof(Value));
static_assert(alignof(CodeFrame) == alignof(Value));

static NativeFunction dummy_function(Context& ctx) {
    auto callback = [&](NativeFunctionFrame& frame) { frame.result(Value::null()); };

    Scope sc(ctx);
    Local name = sc.local(String::make(ctx, "dummy_function"));
    return NativeFunction::make(ctx, name, {}, 0, NativeFunctionArg::sync(callback));
}

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

    CodeFrame user_frame(0, 0, nullptr, *tmpl, {});
    REQUIRE(sizeof(CodeFrame) % sizeof(Value) == 0);
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

TEST_CASE("Coroutine tokens should be cached", "[coroutine]") {
    Context ctx;

    Scope sc(ctx);
    Local func = sc.local(dummy_function(ctx));
    Local coro = sc.local(ctx.make_coroutine(func, {}));
    REQUIRE(coro->current_token().is_null());

    Local token = sc.local(Coroutine::create_token(ctx, coro));
    Local token2 = sc.local(Coroutine::create_token(ctx, coro));
    Local token3 = sc.local(coro->current_token());
    REQUIRE(token->is<CoroutineToken>());
    REQUIRE(token->same(*token2));
    REQUIRE(token->same(*token3));
    REQUIRE(token->coroutine().same(*coro));
    REQUIRE(token->valid());

    // Cannot resume because coroutine is not waiting (did not yield).
    // Successful resume is tested elsewhere (eval).
    REQUIRE_FALSE(CoroutineToken::resume(ctx, token));
}

TEST_CASE("Coroutine tokens should be resettable", "[coroutine]") {
    Context ctx;

    Scope sc(ctx);
    Local func = sc.local(dummy_function(ctx));
    Local coro = sc.local(ctx.make_coroutine(func, {}));
    REQUIRE(coro->current_token().is_null());

    Local first_token = sc.local(Coroutine::create_token(ctx, coro));
    REQUIRE(coro->current_token().same(*first_token));

    coro->reset_token();
    REQUIRE(coro->current_token().is_null());

    Local second_token = sc.local(Coroutine::create_token(ctx, coro));
    REQUIRE(coro->current_token().same(*second_token));
    REQUIRE(second_token->valid());
    REQUIRE_FALSE(first_token->valid());
}

} // namespace tiro::vm::test
