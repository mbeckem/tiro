#include <catch2/catch.hpp>

#include "common/scope_guards.hpp"
#include "vm/context.hpp"
#include "vm/objects/native.hpp"
#include "vm/objects/primitives.hpp"
#include "vm/objects/result.hpp"
#include "vm/objects/string.hpp"

#include <memory>
#include <new>
#include <vector>

namespace tiro::vm::test {

namespace {

template<typename CallbackImpl>
struct SimpleCallback : CoroutineCallback {
    CallbackImpl on_done;

    SimpleCallback(CallbackImpl impl)
        : on_done(std::move(impl)) {}

    SimpleCallback(SimpleCallback&& other) noexcept
        : on_done(std::move(other.on_done)) {}

    void done(Context& ctx, Handle<Coroutine> coro) override { on_done(ctx, coro); }

    void move(void* dest, [[maybe_unused]] size_t size) noexcept override {
        TIRO_DEBUG_ASSERT(dest, "Invalid move destination.");
        TIRO_DEBUG_ASSERT(size == sizeof(*this), "Invalid move destination size.");
        new (dest) SimpleCallback(std::move(*this));
    }

    size_t size() override { return sizeof(SimpleCallback); }

    size_t align() override { return alignof(SimpleCallback); }
};

} // namespace

static void sync_dummy(SyncFrameContext&) {}

static void async_dummy(AsyncFrameContext) {}

static void resumable_dummy(ResumableFrameContext&) {}

TEST_CASE("Native function arg should be trivial", "[native_functions]") {
    static_assert(std::is_trivially_copyable_v<NativeFunctionStorage>);
    static_assert(std::is_trivially_destructible_v<NativeFunctionStorage>);
}

TEST_CASE("Native function arg should wrap sync functions", "[native_functions]") {
    SECTION("Without userdata.") {
        auto arg = NativeFunctionStorage::sync([](SyncFrameContext&) {});

        // Now implemented in terms of resumable functions
        REQUIRE(arg.type() == NativeFunctionType::Resumable);
    }

    SECTION("Static stateless") {
        auto arg = NativeFunctionStorage::static_sync<sync_dummy>();

        // Now implemented in terms of resumable functions
        REQUIRE(arg.type() == NativeFunctionType::Resumable);
    }

    SECTION("With some user data") {
        struct Capture {
            void* x;
            void* y;
        } capture{};

        auto arg = NativeFunctionStorage::sync([capture](SyncFrameContext&) { (void) capture; });

        // Now implemented in terms of resumable functions
        REQUIRE(arg.type() == NativeFunctionType::Resumable);
    }
}

TEST_CASE("Native function arg should wrap async functions", "[native_functions]") {
    SECTION("Without userdata.") {
        auto arg = NativeFunctionStorage::async([](AsyncFrameContext) {});
        REQUIRE(arg.type() == NativeFunctionType::Async);
    }

    SECTION("Static stateless") {
        auto arg = NativeFunctionStorage::static_async<async_dummy>();
        REQUIRE(arg.type() == NativeFunctionType::Async);
    }

    SECTION("With some user data") {
        struct Capture {
            void* x;
            void* y;
        } capture{};

        auto arg = NativeFunctionStorage::async([capture](AsyncFrameContext) { (void) capture; });
        REQUIRE(arg.type() == NativeFunctionType::Async);
    }
}

TEST_CASE("Native function arg should wrap resumable functions", "[native_functions]") {
    SECTION("Without userdata.") {
        auto arg = NativeFunctionStorage::resumable([](ResumableFrameContext&) {});
        REQUIRE(arg.type() == NativeFunctionType::Resumable);
    }

    SECTION("Static stateless") {
        auto arg = NativeFunctionStorage::static_resumable<resumable_dummy>();
        REQUIRE(arg.type() == NativeFunctionType::Resumable);
    }

    SECTION("With some user data") {
        struct Capture {
            void* x;
            void* y;
        } capture{};

        auto arg = NativeFunctionStorage::resumable(
            [capture](ResumableFrameContext&) { (void) capture; });
        REQUIRE(arg.type() == NativeFunctionType::Resumable);
    }
}

TEST_CASE("Native functions should be invocable", "[native_functions]") {
    auto native_func = [](SyncFrameContext& frame) {
        Scope sc(frame.ctx());

        Local values = sc.local(frame.closure());
        Local pointer = sc.local(
            values.must_cast<Tuple>()->checked_get(0).must_cast<NativePointer>());
        int* intptr = static_cast<int*>(pointer->data());
        *intptr = 12345;
        frame.return_value(HeapInteger::make(frame.ctx(), 123));
    };

    Context ctx;
    int i = 0;
    Scope sc(ctx);
    Local func = sc.local<NativeFunction>(defer_init);
    {
        Local name = sc.local(String::make(ctx, "test"));
        Local pointer = sc.local(NativePointer::make(ctx, &i));
        Local values = sc.local(Tuple::make(ctx, 1));
        values->checked_set(0, *pointer);
        func.set(NativeFunction::make(
            ctx, name, values, 0, 0, NativeFunctionStorage::sync(native_func)));
    }

    REQUIRE(func->name().view() == "test");
    REQUIRE(func->params() == 0);

    Local result = sc.local(ctx.run_init(func, {})); // TODO async
    REQUIRE(result->is_success());

    Local value = sc.local(result->unchecked_value());
    REQUIRE(value->must_cast<HeapInteger>().value() == 123);
    REQUIRE(i == 12345);
}

TEST_CASE("Trivial async functions should be invocable", "[native_functions]") {
    // Resumes immediately.
    NativeAsyncFunctionPtr native_func = [](AsyncFrameContext frame) {
        return frame.return_value(SmallInteger::make(3));
    };

    Context ctx;
    Scope sc(ctx);
    Local name = sc.local(String::make(ctx, "Test"));
    Local func = sc.local(
        NativeFunction::make(ctx, name, {}, 0, 0, NativeFunctionStorage::async(native_func)));

    Local result = sc.local(ctx.run_init(func, {}));
    REQUIRE(result->is_success());

    Local value = sc.local(result->unchecked_value());
    REQUIRE(value->must_cast<SmallInteger>().value() == 3);
}

TEST_CASE("Async functions that pause the coroutine should be invocable", "[native_functions]") {
    NativeAsyncFunctionPtr native_func = [](AsyncFrameContext frame) {
        void* loop_ptr = frame.closure().must_cast<NativePointer>().data();
        REQUIRE(loop_ptr);

        auto& loop = *static_cast<std::vector<AsyncFrameContext>*>(loop_ptr);
        loop.push_back(std::move(frame));
    };

    std::vector<AsyncFrameContext> main_loop;
    i64 result = 0;

    Context ctx;
    ScopeExit remove_frames = [&] { main_loop.clear(); }; // Frames must not outlive the context

    Scope sc(ctx);
    Local name = sc.local(String::make(ctx, "Test"));
    Local loop_ptr = sc.local(NativePointer::make(ctx, &main_loop));
    Local func = sc.local(
        NativeFunction::make(ctx, name, loop_ptr, 0, 0, NativeFunctionStorage::async(native_func)));
    Local coro = sc.local(ctx.make_coroutine(func, {}));

    SimpleCallback callback = [&ctx, &result, &coro](
                                  Context& callback_ctx, Handle<Coroutine> callback_coro) {
        REQUIRE(&ctx == &callback_ctx);
        REQUIRE(callback_coro->same(*coro));
        REQUIRE(callback_coro->result().is<Result>());
        REQUIRE(result == 0); // Only called once

        Scope inner(ctx);

        Local coro_result = inner.local(callback_coro->result().must_cast<Result>());
        REQUIRE(coro_result->is_success());

        Local coro_value = inner.local(coro_result->unchecked_value());
        REQUIRE(coro_value->is<SmallInteger>());

        result = coro_value.must_cast<SmallInteger>()->value();
    };
    ctx.set_callback(coro, callback);

    REQUIRE(main_loop.size() == 0);

    ctx.start(coro);
    REQUIRE(main_loop.size() == 0); // Start does not invoke the coroutine
    REQUIRE(ctx.has_ready());

    ctx.run_ready();
    REQUIRE(!ctx.has_ready());
    REQUIRE(main_loop.size() == 1); // Async function was invoked and pushed the frame

    main_loop[0].return_value(SmallInteger::make(123));
    main_loop.clear();
    REQUIRE(ctx.has_ready());

    ctx.run_ready();
    REQUIRE(result == 123); // Coroutine completion callback was executed
}

TEST_CASE("Trivial resumable functions should be invocable", "[native_functions]") {
    NativeResumableFunctionPtr native_func = [](ResumableFrameContext& frame) {
        switch (frame.state()) {
        case ResumableFrame::START:
            return frame.return_value(SmallInteger::make(3));
        }
    };

    Context ctx;
    Scope sc(ctx);
    Local name = sc.local(String::make(ctx, "Test"));
    Local func = sc.local(
        NativeFunction::make(ctx, name, {}, 0, 0, NativeFunctionStorage::resumable(native_func)));

    Local result = sc.local(ctx.run_init(func, {}));
    REQUIRE(result->is_success());

    Local value = sc.local(result->unchecked_value());
    REQUIRE(value->must_cast<SmallInteger>().value() == 3);
}

TEST_CASE("Resumable functions should be able to call other functions", "[native_functions]") {
    enum UserState {
        START = 0,
        AFTER_INVOKE,
    };

    int expected_state = START;
    auto native_resumable_func = [&expected_state](ResumableFrameContext& frame) {
        REQUIRE(frame.state() == expected_state);

        auto& ctx = frame.ctx();
        switch (frame.state()) {
        case ResumableFrame::START: {
            Scope sc(ctx);
            auto func = frame.arg(0);
            auto num = sc.local(HeapInteger::make(ctx, 100));
            auto args = sc.local(Tuple::make(ctx, {num}));

            expected_state = AFTER_INVOKE;
            return frame.invoke(AFTER_INVOKE, *func, *args);
        }
        case AFTER_INVOKE: {
            auto result = frame.invoke_return();
            REQUIRE(result.is<Integer>());
            REQUIRE(result.must_cast<Integer>().value() == 202);
            expected_state = ResumableFrame::CLEANUP;
            return frame.return_value(result);
        }
        }
    };

    Context ctx;
    Scope sc(ctx);
    Local simple_name = sc.local(String::make(ctx, "simple"));
    Local simple_func = sc.local(NativeFunction::make(
        ctx, simple_name, {}, 1, 0, NativeFunctionStorage::sync([](SyncFrameContext& frame) {
            auto num = frame.arg(0);
            REQUIRE(num->is<Integer>());
            i64 result = (num.must_cast<Integer>()->value() * 2) + 2;
            frame.return_value(frame.ctx().get_integer(result));
        })));

    Local resumable_name = sc.local(String::make(ctx, "TestResumable"));
    Local resumable_func = sc.local(NativeFunction::make(
        ctx, resumable_name, {}, 1, 0, NativeFunctionStorage::resumable(native_resumable_func)));
    Local resumable_func_args = sc.local(Tuple::make(ctx, {simple_func}));

    Local result = sc.local(ctx.run_init(resumable_func, resumable_func_args));
    REQUIRE(result->is_success());

    Local value = sc.local(result->unchecked_value());
    REQUIRE(value->must_cast<SmallInteger>().value() == 202);
}

TEST_CASE("Resumable functions should be able to perform cleanup", "[native_functions]") {
    int cleanup_called = 0;
    auto native_func = [&cleanup_called](ResumableFrameContext& frame) {
        switch (frame.state()) {
        case ResumableFrame::START:
            return frame.return_value(SmallInteger::make(3));
        case ResumableFrame::CLEANUP:
            ++cleanup_called;
            break;
        }
    };

    Context ctx;
    Scope sc(ctx);
    Local name = sc.local(String::make(ctx, "Test"));
    Local func = sc.local(
        NativeFunction::make(ctx, name, {}, 0, 0, NativeFunctionStorage::resumable(native_func)));

    Local result = sc.local(ctx.run_init(func, {}));
    REQUIRE(result->is_success());
    REQUIRE(cleanup_called == 1);
}

} // namespace tiro::vm::test
