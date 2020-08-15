#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/native.hpp"
#include "vm/objects/primitives.hpp"
#include "vm/objects/string.hpp"

#include <memory>
#include <new>
#include <vector>

using namespace tiro;
using namespace vm;

namespace {

template<typename CallbackImpl>
struct SimpleCallback : CoroutineCallback {
    CallbackImpl on_done;

    SimpleCallback(CallbackImpl impl)
        : on_done(std::move(impl)) {}

    SimpleCallback(SimpleCallback&& other)
        : on_done(std::move(other.on_done)) {}

    void done(Handle<Coroutine> coro) override { on_done(coro); }

    void move(void* dest, [[maybe_unused]] size_t size) override {
        TIRO_DEBUG_ASSERT(dest, "Invalid move destination.");
        TIRO_DEBUG_ASSERT(size == sizeof(*this), "Invalid move destination size.");
        new (dest) SimpleCallback(std::move(*this));
    }

    size_t size() override { return sizeof(SimpleCallback); }

    size_t align() override { return alignof(SimpleCallback); }
};

} // namespace

TEST_CASE("Native functions should be invokable", "[native_functions]") {

    auto callable = [](NativeFunctionFrame& frame) {
        Scope sc(frame.ctx());

        Local values = sc.local(frame.closure());
        Local pointer = sc.local(values.must_cast<Tuple>()->get(0).must_cast<NativePointer>());
        int* intptr = static_cast<int*>(pointer->data());
        *intptr = 12345;
        frame.result(Integer::make(frame.ctx(), 123));
    };

    Context ctx;
    int i = 0;
    Scope sc(ctx);
    Local func = sc.local<NativeFunction>(defer_init);
    {
        Local name = sc.local(String::make(ctx, "test"));
        Local pointer = sc.local(NativePointer::make(ctx, &i));
        Local values = sc.local(Tuple::make(ctx, 1));
        values->set(0, *pointer);
        func.set(NativeFunction::make(ctx, name, values, 0, callable));
    }

    REQUIRE(func->name().view() == "test");
    REQUIRE(func->params() == 0);

    Local result = sc.local(ctx.run_init(func, {})); // TODO async
    REQUIRE(result->must_cast<Integer>().value() == 123);
    REQUIRE(i == 12345);
}

TEST_CASE("Trivial async functions should be invokable", "[native_functions]") {
    // Resumes immediately.
    NativeAsyncFunctionPtr native_func = [](NativeAsyncFunctionFrame frame) {
        return frame.result(SmallInteger::make(3));
    };

    Context ctx;
    Scope sc(ctx);
    Local name = sc.local(String::make(ctx, "Test"));
    Local func = sc.local(NativeFunction::make(ctx, name, {}, 0, native_func));
    Local result = sc.local(ctx.run_init(func, {}));

    REQUIRE(result->must_cast<SmallInteger>().value() == 3);
}

TEST_CASE("Async functions that pause the coroutine should be invokable", "[native_functions]") {
    NativeAsyncFunctionPtr native_func = [](NativeAsyncFunctionFrame frame) {
        void* loop_ptr = frame.closure().must_cast<NativePointer>().data();
        REQUIRE(loop_ptr);

        auto& loop = *static_cast<std::vector<NativeAsyncFunctionFrame>*>(loop_ptr);
        loop.push_back(std::move(frame));
    };

    std::vector<NativeAsyncFunctionFrame> main_loop;
    i64 result = 0;

    Context ctx;
    ScopeExit remove_frames = [&] { main_loop.clear(); }; // Frames must not outlive the context

    Scope sc(ctx);
    Local name = sc.local(String::make(ctx, "Test"));
    Local loop_ptr = sc.local(NativePointer::make(ctx, &main_loop));
    Local func = sc.local(NativeFunction::make(ctx, name, loop_ptr, 0, native_func));
    Local coro = sc.local(ctx.make_coroutine(func, {}));

    SimpleCallback callback = [&result, &coro](Handle<Coroutine> callback_coro) {
        REQUIRE(callback_coro->same(*coro));
        REQUIRE(callback_coro->result().is<SmallInteger>());
        REQUIRE(result == 0); // Only called once
        result = callback_coro->result().must_cast<SmallInteger>().value();
    };
    ctx.set_callback(coro, callback);

    REQUIRE(main_loop.size() == 0);

    ctx.start(coro);
    REQUIRE(main_loop.size() == 0); // Start does not invoke the coroutine
    REQUIRE(ctx.has_ready());

    ctx.run_ready();
    REQUIRE(!ctx.has_ready());
    REQUIRE(main_loop.size() == 1); // Async function was invoked and pushed the frame

    main_loop[0].result(SmallInteger::make(123));
    REQUIRE(ctx.has_ready());

    ctx.run_ready();
    REQUIRE(result == 123); // Coroutine completion callback was executed
}
