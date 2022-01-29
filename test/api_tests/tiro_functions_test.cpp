#include <catch2/catch.hpp>

#include "tiro/api.h"

#include "./helpers.hpp"

#include <string>
#include <vector>

static void dummy_sync_func(tiro_vm_t, tiro_sync_frame_t){};

static void dummy_async_func(tiro_vm_t, tiro_async_frame_t frame) {
    tiro_async_frame_free(frame);
}

static void dummy_resumable_func(tiro_vm_t, tiro_resumable_frame_t frame) {
    if (tiro_resumable_frame_state(frame) == TIRO_RESUMABLE_STATE_START)
        tiro_resumable_frame_panic_msg(frame, tiro_cstr("error!"), tiro::error_adapter());
}

TEST_CASE("Native sync function construction should fail if invalid arguments are used", "[api]") {
    tiro::vm vm;
    tiro::handle result = tiro::make_null(vm);
    tiro::handle name = tiro::make_string(vm, "func");
    tiro::handle closure = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(nullptr, name.raw_handle(), dummy_sync_func, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name (null handle)") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(vm.raw_vm(), nullptr, dummy_sync_func, 0, closure.raw_handle(),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name (not a string)") {
        tiro::handle number = tiro::make_integer(vm, 123);
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(vm.raw_vm(), number.raw_handle(), dummy_sync_func, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Invalid result handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), dummy_sync_func, 0,
            closure.raw_handle(), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid function pointer") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), nullptr, 0, closure.raw_handle(),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Parameter count too large") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), dummy_sync_func, 1025,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Native sync function construction should succeed", "[api]") {
    tiro::vm vm;
    tiro::handle name = tiro::make_string(vm, "func");
    tiro::handle closure = tiro::make_tuple(vm, 3);
    tiro::handle result = tiro::make_null(vm);

    SECTION("With closure") {
        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), dummy_sync_func, 0,
            closure.raw_handle(), result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_FUNCTION);
    }

    SECTION("Closure is optional") {
        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), dummy_sync_func, 0, nullptr,
            result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_FUNCTION);
    }
}

TEST_CASE("Native sync function invocation should succeed", "[api]") {
    struct Context {
        int called = 0;
        std::exception_ptr error;
    };

    constexpr tiro_sync_function_t native_func = [](tiro_vm_t raw_vm, tiro_sync_frame_t frame) {
        try {
            tiro::vm& vm = tiro::vm::unsafe_from_raw_vm(raw_vm);
            Context* context = std::any_cast<Context*>(vm.userdata());
            try {
                context->called++;

                // Retrieve and verify function arguments.
                REQUIRE(tiro_sync_frame_argc(frame) == 2);
                tiro::handle arg_1 = tiro::make_null(vm);
                tiro::handle arg_2 = tiro::make_null(vm);
                tiro_sync_frame_arg(frame, 0, arg_1.raw_handle(), tiro::error_adapter());
                tiro_sync_frame_arg(frame, 1, arg_2.raw_handle(), tiro::error_adapter());
                REQUIRE(tiro_value_kind(raw_vm, arg_1.raw_handle()) == TIRO_KIND_INTEGER);
                REQUIRE(tiro_value_kind(raw_vm, arg_2.raw_handle()) == TIRO_KIND_FLOAT);

                // Out of bounds errors:
                {
                    tiro::handle result = tiro::make_null(vm);
                    tiro_errc_t errc = TIRO_OK;
                    tiro_sync_frame_arg(frame, 2, result.raw_handle(), error_observer(errc));
                    REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
                }

                // Retrieve captured integer from tuple.
                tiro::handle closure = tiro::make_null(vm);
                tiro_sync_frame_closure(frame, closure.raw_handle(), tiro::error_adapter());
                REQUIRE(tiro_value_kind(raw_vm, closure.raw_handle()) == TIRO_KIND_TUPLE);

                // Perform the actual work (a * b + c).
                tiro::integer closure_value = closure.as<tiro::tuple>().get(0).as<tiro::integer>();
                tiro::float_ result = tiro::make_float(
                    vm, arg_1.as<tiro::integer>().value() * arg_2.as<tiro::float_>().value()
                            + closure_value.value());
                tiro_sync_frame_return_value(frame, result.raw_handle(), tiro::error_adapter());
            } catch (...) {
                context->error = std::current_exception();
            }
        } catch (...) {
            // Some kind of memory corruption.
            std::terminate();
        }
    };

    Context context;
    {
        tiro::vm vm;
        vm.userdata() = &context;

        tiro::string name = tiro::make_string(vm, "func");
        tiro::tuple closure = tiro::make_tuple(vm, 1);
        closure.set(0, tiro::make_integer(vm, 7));

        tiro::handle func = tiro::make_null(vm);
        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), native_func, 2,
            closure.raw_handle(), func.raw_handle(), tiro::error_adapter());

        tiro::tuple args = tiro::make_tuple(vm, 2);
        args.set(0, tiro::make_integer(vm, 10));
        args.set(1, tiro::make_float(vm, 2.5));

        Context coro_context;
        tiro::coroutine coro = tiro::make_coroutine(vm, func.as<tiro::function>(), args);
        coro.set_callback([&coro_context](tiro::vm&, tiro::coroutine inner_coro) {
            coro_context.called += 1;
            try {
                REQUIRE(inner_coro.completed());

                tiro::handle result = inner_coro.result();
                REQUIRE(result.kind() == tiro::value_kind::result);

                tiro::handle value = result.as<tiro::result>().value();
                REQUIRE(value.as<tiro::float_>().value() == 32);
            } catch (...) {
                coro_context.error = std::current_exception();
            }
        });
        coro.start();
        vm.run_ready();

        REQUIRE(coro_context.called == 1);
        if (coro_context.error)
            std::rethrow_exception(context.error);
    }

    REQUIRE(context.called == 1);
    if (context.error)
        std::rethrow_exception(context.error);
}

TEST_CASE("Native sync functions should support panics", "[api]") {
    struct Context {
        std::exception_ptr error;
    } context;

    tiro::vm vm;
    vm.userdata() = &context;

    constexpr tiro_sync_function_t func = [](tiro_vm_t frame_vm, tiro_sync_frame_t frame) {
        tiro::vm& inner_vm = tiro::vm::unsafe_from_raw_vm(frame_vm);
        try {
            tiro_errc_t errc = TIRO_OK;
            tiro_sync_frame_panic_msg(
                frame, tiro_cstr("error from native function"), error_observer(errc));
        } catch (...) {
            std::any_cast<Context*>(inner_vm.userdata())->error = std::current_exception();
        }
    };

    tiro::string name = tiro::make_string(vm, "func");
    tiro::handle function = tiro::make_null(vm);
    tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), func, 0, nullptr, function.raw_handle(),
        tiro::error_adapter());

    tiro::handle result = run_sync(vm, function.as<tiro::function>(), tiro::make_null(vm));
    if (context.error) {
        std::rethrow_exception(context.error);
    }

    REQUIRE(result.kind() == tiro::value_kind::result);

    tiro::handle error = result.as<tiro::result>().error();
    REQUIRE(error.kind() == tiro::value_kind::exception);

    std::string message = error.as<tiro::exception>().message().value();
    REQUIRE(message == "error from native function");
}

TEST_CASE("Sync frame functions should fail for invalid input", "[api]") {
    tiro::vm vm;

    SECTION("Invalid frame for argc") {
        size_t argc = tiro_sync_frame_argc(nullptr);
        REQUIRE(argc == 0);
    }

    SECTION("Invalid frame for arg") {
        tiro::handle result = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_sync_frame_arg(nullptr, 0, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid frame for closure") {
        tiro::handle result = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_sync_frame_closure(nullptr, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid frame for result") {
        tiro::handle value = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_sync_frame_return_value(nullptr, value.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Native async function construction should fail if invalid arguments are used", "[api]") {
    tiro::vm vm;
    tiro::handle result = tiro::make_null(vm);
    tiro::handle name = tiro::make_string(vm, "func");
    tiro::handle closure = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(nullptr, name.raw_handle(), dummy_async_func, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name (null handle)") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(vm.raw_vm(), nullptr, dummy_async_func, 0, closure.raw_handle(),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name (not a string)") {
        tiro::handle number = tiro::make_integer(vm, 123);
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(vm.raw_vm(), number.raw_handle(), dummy_async_func, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Invalid result handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), dummy_async_func, 0,
            closure.raw_handle(), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid function pointer") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), nullptr, 0, closure.raw_handle(),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Parameter count too large") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), dummy_async_func, 1025,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Native async function construction should succeed", "[api]") {
    tiro::vm vm;
    tiro::handle name = tiro::make_string(vm, "func");
    tiro::handle closure = tiro::make_tuple(vm, 3);
    tiro::handle result = tiro::make_null(vm);

    SECTION("With closure") {
        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), dummy_async_func, 0,
            closure.raw_handle(), result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_FUNCTION);
    }

    SECTION("Closure is optional") {
        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), dummy_async_func, 0, nullptr,
            result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_FUNCTION);
    }
}

TEST_CASE("Native async function invocation should succeed", "[api]") {
    struct Task {
        virtual ~Task() = default;
        virtual void run(tiro::vm& vm) = 0;
    };

    struct AsyncContext {
        int called = 0;
        std::exception_ptr error;
        std::vector<std::unique_ptr<Task>> queue;
    };

    struct AsyncTask : Task {
        tiro_async_frame_t frame_;
        double result_;

        AsyncTask(tiro_async_frame_t frame, double result)
            : frame_(frame)
            , result_(result) {}

        ~AsyncTask() { tiro_async_frame_free(frame_); }

        void run(tiro::vm& vm) override {
            tiro::handle result = tiro::make_float(vm, result_);
            tiro_async_frame_return_value(frame_, result.raw_handle(), tiro::error_adapter());
        }
    };

    struct CoroContext {
        int called = 0;
        std::exception_ptr error;
    };

    constexpr tiro_async_function_t native_func = [](tiro_vm_t raw_vm, tiro_async_frame_t frame) {
        try {
            tiro::vm& vm = tiro::vm::unsafe_from_raw_vm(raw_vm);
            AsyncContext* context = std::any_cast<AsyncContext*>(vm.userdata());
            try {
                context->called++;

                // Retrieve and verify function arguments.
                REQUIRE(tiro_async_frame_argc(frame) == 2);
                tiro::handle arg_1 = tiro::make_null(vm);
                tiro::handle arg_2 = tiro::make_null(vm);
                tiro_async_frame_arg(frame, 0, arg_1.raw_handle(), tiro::error_adapter());
                tiro_async_frame_arg(frame, 1, arg_2.raw_handle(), tiro::error_adapter());
                REQUIRE(tiro_value_kind(raw_vm, arg_1.raw_handle()) == TIRO_KIND_INTEGER);
                REQUIRE(tiro_value_kind(raw_vm, arg_2.raw_handle()) == TIRO_KIND_FLOAT);

                // Out of bounds errors:
                {
                    tiro::handle result = tiro::make_null(vm);
                    tiro_errc_t errc = TIRO_OK;
                    tiro_async_frame_arg(frame, 2, result.raw_handle(), error_observer(errc));
                    REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
                }

                // Retrieve captured integer from tuple.
                tiro::handle closure = tiro::make_null(vm);
                tiro_async_frame_closure(frame, closure.raw_handle(), tiro::error_adapter());
                REQUIRE(tiro_value_kind(raw_vm, closure.raw_handle()) == TIRO_KIND_TUPLE);

                // Perform the actual work (a * b + c).
                double result = arg_1.as<tiro::integer>().value() * arg_2.as<tiro::float_>().value()
                                + closure.as<tiro::tuple>().get(0).as<tiro::integer>().value();

                // Enqueue a task to resume the coroutine.
                auto task = std::make_unique<AsyncTask>(frame, result);
                frame = nullptr;
                context->queue.push_back(std::move(task));
            } catch (...) {
                context->error = std::current_exception();
            }
        } catch (...) {
            // Some kind of memory corruption.
            std::terminate();
        }

        if (frame)
            tiro_async_frame_free(frame);
    };

    AsyncContext async_context;
    {
        tiro::vm vm;
        vm.userdata() = &async_context;

        // Pointers to frames must not survive the vm!
        struct CleanupContext {
            ~CleanupContext() { ctx.queue.clear(); }

            AsyncContext& ctx;
        } cleanup_context{async_context};

        tiro::string name = tiro::make_string(vm, "func");
        tiro::tuple closure = tiro::make_tuple(vm, 1);
        closure.set(0, tiro::make_integer(vm, 7));
        tiro::handle func = tiro::make_null(vm);

        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), native_func, 2,
            closure.raw_handle(), func.raw_handle(), tiro::error_adapter());

        tiro::tuple args = tiro::make_tuple(vm, 2);
        args.set(0, tiro::make_integer(vm, 10));
        args.set(1, tiro::make_float(vm, 2.5));

        CoroContext coro_context;
        tiro::coroutine coro = tiro::make_coroutine(vm, func.as<tiro::function>(), args);
        coro.set_callback([&coro_context](tiro::vm&, tiro::coroutine inner_coro) {
            coro_context.called += 1;
            try {
                REQUIRE(inner_coro.completed());

                tiro::handle result = inner_coro.result();
                REQUIRE(result.kind() == tiro::value_kind::result);

                tiro::handle value = result.as<tiro::result>().value();
                REQUIRE(value.as<tiro::float_>().value() == 32);
            } catch (...) {
                coro_context.error = std::current_exception();
            }
        });
        coro.start();

        // Async work is started but not finished. Instead, a task is placed into the queue.
        vm.run_ready();
        REQUIRE(coro_context.called == 0);
        REQUIRE(!vm.has_ready());
        REQUIRE(async_context.called == 1);
        if (async_context.error)
            std::rethrow_exception(async_context.error);

        // Execute the task - this will resume the coroutine.
        REQUIRE(async_context.queue.size() == 1);
        async_context.queue[0]->run(vm);
        async_context.queue.pop_back();

        // The coroutine callback must be executed now.
        REQUIRE(vm.has_ready());
        vm.run_ready();
        REQUIRE(coro_context.called == 1);
        if (coro_context.error)
            std::rethrow_exception(coro_context.error);
    }

    REQUIRE(async_context.called == 1);
    if (async_context.error)
        std::rethrow_exception(async_context.error);
}

TEST_CASE("Native async functions should support panics", "[api]") {
    struct Context {
        std::exception_ptr error;
    } context;

    tiro::vm vm;
    vm.userdata() = &context;

    constexpr tiro_async_function_t func = [](tiro_vm_t frame_vm, tiro_async_frame_t frame) {
        tiro::vm& inner_vm = tiro::vm::unsafe_from_raw_vm(frame_vm);
        try {
            tiro_errc_t errc = TIRO_OK;
            tiro_async_frame_panic_msg(
                frame, tiro_cstr("error from native function"), error_observer(errc));
        } catch (...) {
            std::any_cast<Context*>(inner_vm.userdata())->error = std::current_exception();
        }
        tiro_async_frame_free(frame);
    };

    tiro::string name = tiro::make_string(vm, "func");
    tiro::handle function = tiro::make_null(vm);
    tiro_make_async_function(vm.raw_vm(), name.raw_handle(), func, 0, nullptr,
        function.raw_handle(), tiro::error_adapter());

    tiro::handle result = run_sync(vm, function.as<tiro::function>(), tiro::make_null(vm));
    if (context.error) {
        std::rethrow_exception(context.error);
    }

    REQUIRE(result.kind() == tiro::value_kind::result);

    tiro::handle error = result.as<tiro::result>().error();
    REQUIRE(error.kind() == tiro::value_kind::exception);

    std::string message = error.as<tiro::exception>().message().value();
    REQUIRE(message == "error from native function");
}

TEST_CASE("Async frame functions should fail for invalid input.", "[api]") {
    tiro::vm vm;

    SECTION("Invalid frame for vm") {
        tiro_vm_t raw_vm = tiro_async_frame_vm(nullptr);
        REQUIRE(raw_vm == nullptr);
    }

    SECTION("Invalid frame for argc") {
        size_t argc = tiro_async_frame_argc(nullptr);
        REQUIRE(argc == 0);
    }

    SECTION("Invalid frame for arg") {
        tiro::handle result = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_async_frame_arg(nullptr, 0, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid frame for closure") {
        tiro::handle result = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_async_frame_closure(nullptr, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid frame for result") {
        tiro::handle value = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_async_frame_return_value(nullptr, value.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE(
    "Native resumable function construction should fail if invalid arguments are used", "[api]") {
    tiro::vm vm;
    tiro::handle result = tiro::make_null(vm);
    tiro::handle name = tiro::make_string(vm, "func");
    tiro::handle closure = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_resumable_function(nullptr, name.raw_handle(), dummy_resumable_func, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name (null handle)") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_resumable_function(vm.raw_vm(), nullptr, dummy_resumable_func, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name (not a string)") {
        tiro::handle number = tiro::make_integer(vm, 123);
        tiro_errc_t errc = TIRO_OK;
        tiro_make_resumable_function(vm.raw_vm(), number.raw_handle(), dummy_resumable_func, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Invalid result handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_resumable_function(vm.raw_vm(), name.raw_handle(), dummy_resumable_func, 0,
            closure.raw_handle(), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid function pointer") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_resumable_function(vm.raw_vm(), name.raw_handle(), nullptr, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Parameter count too large") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_resumable_function(vm.raw_vm(), name.raw_handle(), dummy_resumable_func, 1025,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Native resumable function construction should succeed when valid arguments are used",
    "[api]") {
    tiro::vm vm;
    tiro::handle name = tiro::make_string(vm, "func");
    tiro::handle closure = tiro::make_tuple(vm, 3);
    tiro::handle result = tiro::make_null(vm);

    SECTION("With closure") {
        tiro_make_resumable_function(vm.raw_vm(), name.raw_handle(), dummy_resumable_func, 0,
            closure.raw_handle(), result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_FUNCTION);
    }

    SECTION("Closure is optional") {
        tiro_make_resumable_function(vm.raw_vm(), name.raw_handle(), dummy_resumable_func, 0,
            nullptr, result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_FUNCTION);
    }
}

TEST_CASE("Resumable frame functions should fail for invalid input", "[api]") {
    tiro::vm vm;
    // TODO
}

TEST_CASE("Native resumable function invocation should succeed", "[api]") {
    struct CallContext {
        bool panic = false;
        std::vector<int> states;
        std::exception_ptr error;
    };

    constexpr tiro_resumable_function_t native_func = [](tiro_vm_t raw_vm,
                                                          tiro_resumable_frame_t frame) {
        tiro::vm& vm = tiro::vm::unsafe_from_raw_vm(raw_vm);
        CallContext& ctx = *std::any_cast<CallContext*>(vm.userdata());
        if (ctx.error)
            return;

        try {
            int state = tiro_resumable_frame_state(frame);
            ctx.states.push_back(state);
            switch (state) {
            case TIRO_RESUMABLE_STATE_START:
                return tiro_resumable_frame_set_state(
                    frame, ctx.panic ? 456 : 123, tiro::error_adapter());
            case 123: {
                int argc = tiro_resumable_frame_argc(frame);
                REQUIRE(argc == 2);

                tiro::handle x = tiro::make_null(vm);
                tiro::handle y = tiro::make_null(vm);
                tiro_resumable_frame_arg(frame, 0, x.raw_handle(), tiro::error_adapter());
                tiro_resumable_frame_arg(frame, 1, y.raw_handle(), tiro::error_adapter());
                REQUIRE(tiro_value_kind(raw_vm, x.raw_handle()) == TIRO_KIND_INTEGER);
                REQUIRE(tiro_value_kind(raw_vm, y.raw_handle()) == TIRO_KIND_FLOAT);

                tiro::handle closure = tiro::make_null(vm);
                tiro_resumable_frame_closure(frame, closure.raw_handle(), tiro::error_adapter());
                REQUIRE(tiro_value_kind(raw_vm, closure.raw_handle()) == TIRO_KIND_TUPLE);
                tiro::handle z = closure.as<tiro::tuple>().get(0);

                tiro::float_ result = tiro::make_float(
                    vm, x.as<tiro::integer>().value() * y.as<tiro::float_>().value()
                            + z.as<tiro::integer>().value());
                return tiro_resumable_frame_return_value(
                    frame, result.raw_handle(), tiro::error_adapter());
            }
            case 456:
                return tiro_resumable_frame_panic_msg(
                    frame, tiro_cstr("custom panic message"), tiro::error_adapter());
            case TIRO_RESUMABLE_STATE_END:
            case TIRO_RESUMABLE_STATE_CLEANUP:
                return;
            }
        } catch (...) {
            ctx.error = std::current_exception();
            tiro_resumable_frame_panic_msg(frame, tiro_cstr("internal error"), nullptr);
        }
    };

    CallContext call_context;
    tiro::vm vm;
    vm.userdata() = &call_context;

    tiro::string name = tiro::make_string(vm, "func");
    tiro::tuple closure = tiro::make_tuple(vm, 1);
    closure.set(0, tiro::make_integer(vm, 7));

    tiro::handle func = tiro::make_null(vm);
    tiro_make_resumable_function(vm.raw_vm(), name.raw_handle(), native_func, 2,
        closure.raw_handle(), func.raw_handle(), tiro::error_adapter());

    tiro::tuple args = tiro::make_tuple(vm, 2);
    args.set(0, tiro::make_integer(vm, 10));
    args.set(1, tiro::make_float(vm, 2.5));

    SECTION("when returning normally") {
        tiro::result result = run_sync(vm, func.as<tiro::function>(), args);
        if (call_context.error)
            std::rethrow_exception(call_context.error);

        REQUIRE(result.is_success());
        tiro::handle value = result.value();
        REQUIRE(value.is<tiro::float_>());
        REQUIRE(value.as<tiro::float_>().value() == 32.0);
    }

    SECTION("when panicking") {
        call_context.panic = true;
        tiro::result result = run_sync(vm, func.as<tiro::function>(), args);
        if (call_context.error)
            std::rethrow_exception(call_context.error);
        REQUIRE(result.is_error());

        tiro::exception panic = result.error().as<tiro::exception>();
        REQUIRE(panic.message().value() == "custom panic message");
    }
}
