#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/all.hpp"
#include "vm/objects/coroutine.hpp"

namespace tiro::vm::test {

static NativeFunction dummy_function(Context& ctx) {
    auto callback = [&](SyncFrameContext& frame) { frame.return_value(Value::null()); };

    Scope sc(ctx);
    Local name = sc.local(String::make(ctx, "dummy_function"));
    return NativeFunction::sync(callback).name(name).make(ctx);
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
