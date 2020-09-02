#include "api/internal.hpp"

#include "vm/math.hpp"
#include "vm/objects/all.hpp"

#include <new>

using namespace tiro;
using namespace tiro::api;

const char* tiro_kind_str(tiro_kind_t kind) {
    switch (kind) {
#define TIRO_CASE(Kind)    \
    case TIRO_KIND_##Kind: \
        return #Kind;

        TIRO_CASE(NULL)
        TIRO_CASE(BOOLEAN)
        TIRO_CASE(INTEGER)
        TIRO_CASE(FLOAT)
        TIRO_CASE(STRING)
        TIRO_CASE(FUNCTION)
        TIRO_CASE(TUPLE)
        TIRO_CASE(ARRAY)
        TIRO_CASE(RESULT)
        TIRO_CASE(COROUTINE)
        TIRO_CASE(MODULE)
        TIRO_CASE(TYPE)
        TIRO_CASE(NATIVE)
        TIRO_CASE(INTERNAL)
        TIRO_CASE(INVALID)

#undef TIRO_KIND
    }

    return "<INVALID KIND>";
}

tiro_kind_t tiro_value_kind(tiro_vm_t vm, tiro_handle_t value) {
    if (!vm || !value)
        return TIRO_KIND_INVALID;

    // TODO: These can also be derived from the public types!
    auto handle = to_internal(value);
    switch (handle->type()) {
#define TIRO_MAP(VmType, Kind)  \
    case vm::ValueType::VmType: \
        return TIRO_KIND_##Kind;

        TIRO_MAP(Null, NULL)
        TIRO_MAP(Boolean, BOOLEAN)
        TIRO_MAP(SmallInteger, INTEGER)
        TIRO_MAP(Integer, INTEGER)
        TIRO_MAP(Float, FLOAT)
        TIRO_MAP(String, STRING)
        TIRO_MAP(Tuple, TUPLE)
        TIRO_MAP(BoundMethod, FUNCTION)
        TIRO_MAP(Function, FUNCTION)
        TIRO_MAP(NativeFunction, FUNCTION)
        TIRO_MAP(Array, ARRAY)
        TIRO_MAP(Result, RESULT)
        TIRO_MAP(Coroutine, COROUTINE)
        TIRO_MAP(Module, MODULE)
        TIRO_MAP(Type, TYPE)
        TIRO_MAP(NativeObject, NATIVE)

    default:
        return TIRO_KIND_INTERNAL;

#undef TIRO_MAP
    }
}

static std::optional<vm::ValueType> get_type(tiro_kind_t kind) {
    switch (kind) {
#define TIRO_MAP(Kind, VmType) \
    case TIRO_KIND_##Kind:     \
        return vm::ValueType::VmType;

        TIRO_MAP(NULL, Null)
        TIRO_MAP(BOOLEAN, Boolean)
        TIRO_MAP(INTEGER, Integer)
        TIRO_MAP(FLOAT, Float)
        TIRO_MAP(STRING, String)
        TIRO_MAP(TUPLE, Tuple)
        TIRO_MAP(FUNCTION, Function)
        TIRO_MAP(ARRAY, Array)
        TIRO_MAP(RESULT, Result)
        TIRO_MAP(COROUTINE, Coroutine)
        TIRO_MAP(MODULE, Module)
        TIRO_MAP(TYPE, Type)
        TIRO_MAP(NATIVE, NativeObject)

    default:
        return {};

#undef TIRO_MAP
    }
};

static vm::NativeFunctionArg function_arg(tiro_sync_function_t sync_func) {
    struct Function {
        tiro_sync_function_t func;

        void operator()(vm::NativeFunctionFrame& frame) {
            func(vm_from_context(frame.ctx()), to_external(&frame));
        }
    };

    return vm::NativeFunctionArg::sync(Function{sync_func});
}

static vm::NativeFunctionArg function_arg(tiro_async_function_t async_func) {
    struct Function {
        tiro_async_function_t func;

        void operator()(vm::NativeAsyncFunctionFrame frame) {
            auto dynamic_frame = std::make_unique<vm::NativeAsyncFunctionFrame>(std::move(frame));
            func(vm_from_context(frame.ctx()), to_external(dynamic_frame.release()));
        }
    };

    return vm::NativeFunctionArg::async(Function{async_func});
}

template<typename FunctionPtr>
static void make_native_function(tiro_vm_t vm, tiro_handle_t name, FunctionPtr func, size_t argc,
    tiro_handle_t closure, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !name || !func || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        // Internally argc is uint32_t, this must always be smaller than 2 ** 32.
        static constexpr size_t max_argc = 1024;
        if (argc > max_argc)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_name = to_internal(name).try_cast<vm::String>();
        if (!maybe_name)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto name_handle = maybe_name.handle();
        auto maybe_closure = to_internal_maybe(closure);
        auto result_handle = to_internal(result);

        result_handle.set(vm::NativeFunction::make(
            ctx, name_handle, maybe_closure, static_cast<u32>(argc), function_arg(func)));
    });
}

void tiro_value_type(tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !value || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto value_handle = to_internal(value);
        auto result_handle = to_internal(result);
        result_handle.set(ctx.types().type_of(value_handle));
    });
}

void tiro_value_to_string(
    tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !value || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto value_handle = to_internal(value);
        auto result_handle = to_internal(result);

        vm::Scope sc(ctx);
        vm::Local builder = sc.local(vm::StringBuilder::make(ctx, 32));
        vm::to_string(ctx, builder, value_handle);
        result_handle.set(builder->to_string(ctx));
    });
}

void tiro_value_copy(tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !value || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto value_handle = to_internal(value);
        auto result_handle = to_internal(result);
        result_handle.set(value_handle);
    });
}

void tiro_kind_type(tiro_vm_t vm, tiro_kind_t kind, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto vm_type = get_type(kind);
        if (!vm_type)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto result_handle = to_internal(result);
        result_handle.set(ctx.types().type_of(*vm_type));
    });
}

void tiro_make_null(tiro_vm_t vm, tiro_handle_t result) {
    return entry_point(nullptr, [&] {
        if (!vm || !result)
            return;

        to_internal(result).set(vm::Value::null());
    });
}

void tiro_make_boolean(tiro_vm_t vm, bool value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto result_handle = to_internal(result);
        result_handle.set(ctx.get_boolean(value));
    });
}

bool tiro_boolean_value(tiro_vm_t vm, tiro_handle_t value) {
    return entry_point(nullptr, false, [&] {
        if (!vm)
            return false;

        vm::Context& ctx = vm->ctx;
        return ctx.is_truthy(to_internal(value));
    });
}

void tiro_make_integer(tiro_vm_t vm, int64_t value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto result_handle = to_internal(result);
        result_handle.set(ctx.get_integer(value));
    });
}

int64_t tiro_integer_value(tiro_vm_t vm, tiro_handle_t value) {
    return entry_point(nullptr, 0, [&]() -> int64_t {
        if (!vm || !value)
            return 0;

        auto handle = to_internal(value);
        if (auto i = vm::try_convert_integer(*handle))
            return *i;
        return 0;
    });
}

void tiro_make_float(tiro_vm_t vm, double value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto result_handle = to_internal(result);
        result_handle.set(vm::Float::make(ctx, value));
    });
}

double tiro_float_value(tiro_vm_t vm, tiro_handle_t value) {
    return entry_point(nullptr, 0, [&]() -> double {
        if (!vm || !value)
            return 0;

        auto handle = to_internal(value);
        if (auto f = vm::try_convert_float(*handle))
            return *f;

        return 0;
    });
}

void tiro_make_string(tiro_vm_t vm, const char* value, tiro_handle_t result, tiro_error_t* err) {
    return tiro_make_string_from_data(vm, value, value != NULL ? strlen(value) : 0, result, err);
}

void tiro_make_string_from_data(
    tiro_vm_t vm, const char* data, size_t length, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result || (!data && length > 0))
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto result_handle = to_internal(result);
        result_handle.set(vm::String::make(ctx, std::string_view(data, length)));
    });
}

void tiro_string_value(
    tiro_vm_t vm, tiro_handle_t string, const char** data, size_t* length, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !string || !data || !length)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_string_handle = to_internal(string).try_cast<vm::String>();
        if (!maybe_string_handle)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto string_handle = maybe_string_handle.handle();
        auto storage = string_handle->view();
        *data = storage.data();
        *length = storage.length();
    });
}

void tiro_string_cstr(tiro_vm_t vm, tiro_handle_t string, char** result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !string || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_string_handle = to_internal(string).try_cast<vm::String>();
        if (!maybe_string_handle)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto string_handle = maybe_string_handle.handle();
        *result = copy_to_cstr(string_handle->view());
    });
}

void tiro_make_tuple(tiro_vm_t vm, size_t size, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto result_handle = to_internal(result);
        result_handle.set(vm::Tuple::make(ctx, size));
    });
}

size_t tiro_tuple_size(tiro_vm_t vm, tiro_handle_t tuple) {
    return entry_point(nullptr, 0, [&]() -> size_t {
        if (!vm || !tuple)
            return 0;

        auto maybe_tuple = to_internal(tuple).try_cast<vm::Tuple>();
        if (!maybe_tuple)
            return 0;

        return maybe_tuple.handle()->size();
    });
}

void tiro_tuple_get(
    tiro_vm_t vm, tiro_handle_t tuple, size_t index, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !tuple || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_tuple = to_internal(tuple).try_cast<vm::Tuple>();
        if (!maybe_tuple)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto tuple_handle = maybe_tuple.handle();
        if (size_t size = tuple_handle->size(); index >= size)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        to_internal(result).set(tuple_handle->get(index));
    });
}

void tiro_tuple_set(
    tiro_vm_t vm, tiro_handle_t tuple, size_t index, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !tuple || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_tuple = to_internal(tuple).try_cast<vm::Tuple>();
        if (!maybe_tuple)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto tuple_handle = maybe_tuple.handle();
        if (size_t size = tuple_handle->size(); index >= size)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        tuple_handle->set(index, *to_internal(value));
    });
}

void tiro_make_array(
    tiro_vm_t vm, size_t initial_capacity, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto result_handle = to_internal(result);
        result_handle.set(vm::Array::make(ctx, initial_capacity));
    });
}

size_t tiro_array_size(tiro_vm_t vm, tiro_handle_t array) {
    return entry_point(nullptr, 0, [&]() -> size_t {
        if (!vm || !array)
            return 0;

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return 0;

        return maybe_array.handle()->size();
    });
}

void tiro_array_get(
    tiro_vm_t vm, tiro_handle_t array, size_t index, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !array || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        if (size_t size = array_handle->size(); index >= size)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        to_internal(result).set(array_handle->get(index));
    });
}

void tiro_array_set(
    tiro_vm_t vm, tiro_handle_t array, size_t index, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !array || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        if (size_t size = array_handle->size(); index >= size)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        array_handle->set(index, to_internal(value));
    });
}

void tiro_array_push(tiro_vm_t vm, tiro_handle_t array, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !array || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        array_handle->append(ctx, to_internal(value));
    });
}

void tiro_array_pop(tiro_vm_t vm, tiro_handle_t array, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        if (array_handle->size() == 0)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        array_handle->remove_last();
    });
}

void tiro_array_clear(tiro_vm_t vm, tiro_handle_t array, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        array_handle->clear();
    });
}

void tiro_make_success(tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !value || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto value_handle = to_internal(value);
        auto result_handle = to_internal(result);
        result_handle.set(vm::Result::make_success(ctx, value_handle));
    });
}

void tiro_make_failure(
    tiro_vm_t vm, tiro_handle_t reason, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !reason || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        auto reason_handle = to_internal(reason);
        auto result_handle = to_internal(result);
        result_handle.set(vm::Result::make_failure(ctx, reason_handle));
    });
}

static std::optional<vm::Result::Which> result_which(tiro_vm_t vm, tiro_handle_t handle) noexcept {
    return entry_point(nullptr, std::nullopt, [&]() -> std::optional<vm::Result::Which> {
        if (!vm || !handle)
            return {};

        auto maybe_result = to_internal(handle).try_cast<vm::Result>();
        if (!maybe_result)
            return {};

        return maybe_result.handle()->which();
    });
}

bool tiro_result_is_success(tiro_vm_t vm, tiro_handle_t instance) {
    return result_which(vm, instance) == vm::Result::Success;
}

bool tiro_result_is_failure(tiro_vm_t vm, tiro_handle_t instance) {
    return result_which(vm, instance) == vm::Result::Failure;
}

void tiro_result_value(tiro_vm_t vm, tiro_handle_t instance, tiro_handle_t out, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !instance || !out)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_result = to_internal(instance).try_cast<vm::Result>();
        if (!maybe_result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto result_handle = maybe_result.handle();
        if (!result_handle->is_success())
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        auto out_handle = to_internal(out);
        out_handle.set(result_handle->value());
    });
}

void tiro_result_reason(
    tiro_vm_t vm, tiro_handle_t instance, tiro_handle_t out, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !instance || !out)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_result = to_internal(instance).try_cast<vm::Result>();
        if (!maybe_result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto result_handle = maybe_result.handle();
        if (!result_handle->is_failure())
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        auto out_handle = to_internal(out);
        out_handle.set(result_handle->reason());
    });
}

void tiro_make_coroutine(tiro_vm_t vm, tiro_handle_t func, tiro_handle_t arguments,
    tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !func || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (tiro_value_kind(vm, func) != TIRO_KIND_FUNCTION)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        vm::Context& ctx = vm->ctx;
        auto func_handle = to_internal(func);
        auto args_handle = to_internal_maybe(arguments);
        auto result_handle = to_internal(result);

        if (args_handle) {
            auto args = args_handle.handle();
            if (!args->is<vm::Null>() && !args->is<vm::Tuple>())
                return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);
        }

        result_handle.set(ctx.make_coroutine(func_handle, args_handle.try_cast<vm::Tuple>()));
    });
}

bool tiro_coroutine_started(tiro_vm_t vm, tiro_handle_t coroutine) {
    return entry_point(nullptr, false, [&] {
        if (!vm || !coroutine)
            return false;

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return false;

        return maybe_coro.handle()->state() != vm::CoroutineState::New;
    });
}

bool tiro_coroutine_completed(tiro_vm_t vm, tiro_handle_t coroutine) {
    return entry_point(nullptr, false, [&] {
        if (!vm || !coroutine)
            return false;

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return false;

        return maybe_coro.handle()->state() == vm::CoroutineState::Done;
    });
}

void tiro_coroutine_result(
    tiro_vm_t vm, tiro_handle_t coroutine, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !coroutine || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto coro_handle = maybe_coro.handle();
        if (coro_handle->state() != vm::CoroutineState::Done)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        auto result_handle = to_internal(result);
        result_handle.set(coro_handle->result());
    });
}

void tiro_coroutine_set_callback(tiro_vm_t vm, tiro_handle_t coroutine,
    tiro_coroutine_callback callback, tiro_coroutine_cleanup cleanup, void* userdata,
    tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !coroutine || !callback)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        struct Callback final : vm::CoroutineCallback {
            tiro_coroutine_callback callback;
            tiro_coroutine_cleanup cleanup;
            void* userdata;

            Callback(tiro_coroutine_callback callback_, tiro_coroutine_cleanup cleanup_,
                void* userdata_) noexcept
                : callback(callback_)
                , cleanup(cleanup_)
                , userdata(userdata_) {}

            Callback(Callback&& other) noexcept
                : callback(std::exchange(other.callback, nullptr))
                , cleanup(std::exchange(other.cleanup, nullptr))
                , userdata(std::exchange(other.userdata, nullptr)) {}

            ~Callback() {
                if (cleanup)
                    cleanup(userdata);
            }

            void done(vm::Context& ctx, vm::Handle<vm::Coroutine> coroutine) override {
                TIRO_DEBUG_ASSERT(callback, "Cannot invoke callback on invalid instance.");

                // Must create a local handle because the external c api only understands
                // mutable values for now. Improvement: const_handles in the api.
                vm::Scope sc(ctx);
                vm::Local coro = sc.local<vm::Value>(coroutine);
                callback(vm_from_context(ctx), to_external(coro.mut()), userdata);
                if (cleanup) {
                    cleanup(userdata);
                    cleanup = nullptr;
                }
            }

            void move(void* dest, [[maybe_unused]] size_t size) noexcept override {
                TIRO_DEBUG_ASSERT(dest, "Invalid move destination.");
                TIRO_DEBUG_ASSERT(size == sizeof(*this), "Invalid move destination size.");
                new (dest) Callback(std::move(*this));
            }

            size_t size() override { return sizeof(Callback); }

            size_t align() override { return alignof(Callback); }
        };

        vm::Context& ctx = vm->ctx;

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto coro_handle = maybe_coro.handle();

        Callback cb(callback, cleanup, userdata);
        ctx.set_callback(coro_handle, cb);
    });
}

void tiro_coroutine_start(tiro_vm_t vm, tiro_handle_t coroutine, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !coroutine)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto coro_handle = maybe_coro.handle();
        if (coro_handle->state() != vm::CoroutineState::New)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        ctx.start(coro_handle);
    });
}

void tiro_make_module(tiro_vm_t vm, const char* name, tiro_module_member_t* members,
    size_t members_length, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !name || (*name == 0) || (members_length > 0 && !members) || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);
        vm::Local module_name = sc.local(ctx.get_interned_string(name));
        vm::Local export_name = sc.local();
        vm::Local module_exports = sc.local(vm::HashTable::make(ctx));

        for (size_t i = 0; i < members_length; ++i) {
            const char* raw_name = members[i].name;
            tiro_handle_t value = members[i].value;

            if (!raw_name || (*raw_name == 0) || !value)
                return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

            // TODO: Allocate new string instead? Rather make sure that interned strings get garbage collected...
            export_name = ctx.get_symbol(raw_name);
            module_exports->set(ctx, export_name, to_internal(value));
        }

        // Needed by the module creation function.
        vm::Local module_members = sc.local(vm::Tuple::make(ctx, 0));

        auto result_handle = to_internal(result);
        result_handle.set(vm::Module::make(ctx, module_name, module_members, module_exports));
    });
}

void tiro_module_get_export(tiro_vm_t vm, tiro_handle_t module, const char* export_name,
    tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !module || !export_name || (*export_name == 0) || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);

        auto maybe_module = to_internal(module).try_cast<vm::Module>();
        if (!maybe_module)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto module_handle = maybe_module.handle();

        vm::Local export_symbol = sc.local(ctx.get_symbol(export_name));
        if (auto found = module_handle->find_exported(export_symbol)) {
            to_internal(result).set(*found);
        } else {
            return TIRO_REPORT(err, TIRO_ERROR_EXPORT_NOT_FOUND);
        }
    });
}

void tiro_type_name(tiro_vm_t vm, tiro_handle_t type, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !type || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto maybe_type = to_internal(type).try_cast<vm::Type>();
        if (!maybe_type)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto type_handle = maybe_type.handle();
        auto result_handle = to_internal(result);
        result_handle.set(type_handle->name());
    });
}

void tiro_make_native(tiro_vm_t vm, const tiro_native_type_t* type_descriptor, size_t size,
    tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !type_descriptor || size == 0 || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto result_handle = to_internal(result);
        result_handle.set(vm::NativeObject::make(ctx, type_descriptor, size));
    });
}

const tiro_native_type_t* tiro_native_type_descriptor(tiro_vm_t vm, tiro_handle_t object) {
    return entry_point(nullptr, nullptr, [&]() -> const tiro_native_type_t* {
        if (!vm || !object)
            return nullptr;

        auto maybe_object = to_internal(object).try_cast<vm::NativeObject>();
        if (!maybe_object)
            return nullptr;

        return maybe_object.handle()->native_type();
    });
}

void* tiro_native_data(tiro_vm_t vm, tiro_handle_t object) {
    return entry_point(nullptr, nullptr, [&]() -> void* {
        if (!vm || !object)
            return nullptr;

        auto maybe_object = to_internal(object).try_cast<vm::NativeObject>();
        if (!maybe_object)
            return nullptr;

        return maybe_object.handle()->data();
    });
}

size_t tiro_native_size(tiro_vm_t vm, tiro_handle_t object) {
    return entry_point(nullptr, 0, [&]() -> size_t {
        if (!vm || !object)
            return 0;

        auto maybe_object = to_internal(object).try_cast<vm::NativeObject>();
        if (!maybe_object)
            return 0;

        return maybe_object.handle()->size();
    });
}

size_t tiro_sync_frame_argc(tiro_sync_frame_t frame) {
    return entry_point(nullptr, 0, [&]() -> size_t {
        if (!frame)
            return 0;

        return to_internal(frame)->arg_count();
    });
}

void tiro_sync_frame_arg(
    tiro_sync_frame_t frame, size_t index, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        if (index >= internal_frame->arg_count())
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        auto result_handle = to_internal(result);
        result_handle.set(internal_frame->arg(index));
    });
}

void tiro_sync_frame_closure(tiro_sync_frame_t frame, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        auto result_handle = to_internal(result);
        result_handle.set(internal_frame->closure());
    });
}

void tiro_sync_frame_result(tiro_sync_frame_t frame, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        auto value_handle = to_internal(value);
        internal_frame->result(*value_handle);
    });
}

void tiro_make_sync_function(tiro_vm_t vm, tiro_handle_t name, tiro_sync_function_t func,
    size_t argc, tiro_handle_t closure, tiro_handle_t result, tiro_error_t* err) {
    return make_native_function(vm, name, func, argc, closure, result, err);
}

void tiro_async_frame_free(tiro_async_frame_t frame) {
    delete to_internal(frame);
}

tiro_vm_t tiro_async_frame_vm(tiro_async_frame_t frame) {
    return entry_point(nullptr, nullptr, [&]() -> tiro_vm_t {
        if (!frame)
            return nullptr;

        return vm_from_context(to_internal(frame)->ctx());
    });
}

size_t tiro_async_frame_argc(tiro_async_frame_t frame) {
    return entry_point(nullptr, 0, [&]() -> size_t {
        if (!frame)
            return 0;

        return to_internal(frame)->arg_count();
    });
}

void tiro_async_frame_arg(
    tiro_async_frame_t frame, size_t index, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        if (index >= internal_frame->arg_count())
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        auto result_handle = to_internal(result);
        result_handle.set(internal_frame->arg(index));
    });
}

void tiro_async_frame_closure(tiro_async_frame_t frame, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        auto result_handle = to_internal(result);
        result_handle.set(internal_frame->closure());
    });
}

void tiro_async_frame_result(tiro_async_frame_t frame, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        auto value_handle = to_internal(value);
        internal_frame->result(*value_handle);
    });
}

void tiro_make_async_function(tiro_vm_t vm, tiro_handle_t name, tiro_async_function_t func,
    size_t argc, tiro_handle_t closure, tiro_handle_t result, tiro_error_t* err) {
    return make_native_function(vm, name, func, argc, closure, result, err);
}
