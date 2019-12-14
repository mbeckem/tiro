#include <catch.hpp>

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/functions.hpp"
#include "hammer/vm/objects/native_objects.hpp"
#include "hammer/vm/objects/primitives.hpp"
#include "hammer/vm/objects/strings.hpp"

#include <boost/asio/steady_timer.hpp>

#include <memory>

using namespace hammer;
using namespace vm;

TEST_CASE("Native function execution", "[function]") {
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
    REQUIRE(func->min_params() == 0);

    Root result(ctx, ctx.run(func.handle()));
    REQUIRE(result->as<Integer>().value() == 123);
    REQUIRE(i == 12345);
}

TEST_CASE(
    "native async function that resumes immediately", "[native_functions]") {
    NativeAsyncFunction::FunctionType native_func =
        [](NativeAsyncFunction::Frame frame) {
            frame.result(SmallInteger::make(3));
            frame.resume();
        };

    Context ctx;
    Root<Value> func(
        ctx, NativeAsyncFunction::make(ctx, {}, {}, 0, native_func));
    Root<Value> result(ctx, ctx.run(func));

    REQUIRE(result->as<SmallInteger>().value() == 3);
}

TEST_CASE("native async function that resumes later", "[native_functions]") {
    struct TimeoutAction : std::enable_shared_from_this<TimeoutAction> {
        TimeoutAction(
            NativeAsyncFunction::Frame frame, boost::asio::io_context& io)
            : frame_(std::move(frame))
            , timer_(io) {}

        void start() {
            timer_.expires_after(std::chrono::milliseconds(1));
            timer_.async_wait([self = shared_from_this()](std::error_code ec) {
                self->on_expired(ec);
            });
        }

        void on_expired(std::error_code ec) {
            frame_.result(SmallInteger::make(ec ? 1 : 2));
            frame_.resume();
        }

        NativeAsyncFunction::Frame frame_;
        boost::asio::steady_timer timer_;
    };

    NativeAsyncFunction::FunctionType native_func =
        [](NativeAsyncFunction::Frame frame) {
            auto& io = frame.ctx().io_context();
            auto action = std::make_shared<TimeoutAction>(std::move(frame), io);
            action->start();
        };

    Context ctx;
    Root<Value> func(
        ctx, NativeAsyncFunction::make(ctx, {}, {}, 0, native_func));
    Root<Value> result(ctx, ctx.run(func));

    REQUIRE(result->as<SmallInteger>().value() == 2);
}
