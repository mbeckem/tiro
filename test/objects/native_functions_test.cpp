#include <catch.hpp>

#include "tiro/objects/functions.hpp"
#include "tiro/objects/native_objects.hpp"
#include "tiro/objects/primitives.hpp"
#include "tiro/objects/strings.hpp"
#include "tiro/vm/context.hpp"

#include <asio/steady_timer.hpp>

#include <memory>

using namespace tiro;
using namespace vm;

// TODO: Test argument passing to native functions from ctx.run()

TEST_CASE("Native functions should be invokable", "[function]") {
    auto callable = [](NativeFunction::Frame& frame) {
        Context& ctx = frame.ctx();
        Root values(ctx, frame.values());
        Root pointer(ctx, values->get(0).as<NativePointer>());
        int* intptr = static_cast<int*>(pointer->native_ptr());
        *intptr = 12345;
        frame.result(Integer::make(frame.ctx(), 123));
    };

    int i = 0;
    Context ctx;
    Root<NativeFunction> func(ctx);
    {
        Root name(ctx, String::make(ctx, "test"));
        Root pointer(ctx, NativePointer::make(ctx, &i));
        Root values(ctx, Tuple::make(ctx, 1));
        values->set(0, pointer);
        func.set(NativeFunction::make(ctx, name, values, 0, callable));
    }

    REQUIRE(func->name().view() == "test");
    REQUIRE(func->params() == 0);

    Root result(ctx, ctx.run(func.handle(), {}));
    REQUIRE(result->as<Integer>().value() == 123);
    REQUIRE(i == 12345);
}

static void trivial_callback(NativeAsyncFunction::Frame frame) {
    return frame.result(SmallInteger::make(3));
}

TEST_CASE("Trivial async functions should be invokable", "[native_functions]") {
    NativeAsyncFunction::FunctionType native_func = trivial_callback;

    Context ctx;
    Root<Value> func(
        ctx, NativeAsyncFunction::make(ctx, {}, {}, 0, native_func));
    Root<Value> result(ctx, ctx.run(func, {}));

    REQUIRE(result->as<SmallInteger>().value() == 3);
}

TEST_CASE("Async functions that pause the coroutine should be invokable",
    "[native_functions]") {
    struct TimeoutAction : std::enable_shared_from_this<TimeoutAction> {
        TimeoutAction(NativeAsyncFunction::Frame frame, asio::io_context& io)
            : frame_(std::move(frame))
            , timer_(io) {}

        static void callback(NativeAsyncFunction::Frame frame) {
            auto& io = frame.ctx().io_context();
            auto action = std::make_shared<TimeoutAction>(std::move(frame), io);
            action->start();
        };

        void start() {
            timer_.expires_after(std::chrono::milliseconds(1));
            timer_.async_wait([self = shared_from_this()](std::error_code ec) {
                self->on_expired(ec);
            });
        }

        void on_expired(std::error_code ec) {
            return frame_.result(SmallInteger::make(ec ? 1 : 2));
        }

        NativeAsyncFunction::Frame frame_;
        asio::steady_timer timer_;
    };

    NativeAsyncFunction::FunctionType native_func = TimeoutAction::callback;

    Context ctx;
    Root<Value> func(
        ctx, NativeAsyncFunction::make(ctx, {}, {}, 0, native_func));
    Root<Value> result(ctx, ctx.run(func, {}));

    REQUIRE(result->as<SmallInteger>().value() == 2);
}
