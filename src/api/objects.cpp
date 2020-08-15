#include "api/internal.hpp"

#include "vm/math.hpp"
#include "vm/objects/all.hpp"

#include <new>

using namespace tiro;
using namespace tiro::api;

const char* tiro_kind_str(tiro_kind kind) {
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
        TIRO_CASE(COROUTINE)
        TIRO_CASE(TYPE)
        TIRO_CASE(INTERNAL)
        TIRO_CASE(INVALID)

#undef TIRO_KIND
    }

    return "<INVALID KIND>";
}

tiro_kind tiro_value_kind(tiro_vm* vm, tiro_handle value) {
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
        TIRO_MAP(Coroutine, COROUTINE)
        TIRO_MAP(Type, TYPE)

    default:
        return TIRO_KIND_INTERNAL;

#undef TIRO_MAP
    }
}

static std::optional<vm::ValueType> get_type(tiro_kind kind) {
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
        TIRO_MAP(COROUTINE, Coroutine)
        TIRO_MAP(TYPE, Type)

    default:
        return {};

#undef TIRO_MAP
    }
};

tiro_errc tiro_value_type(tiro_vm* vm, tiro_handle value, tiro_handle result, tiro_error** err) {
    if (!vm || !value || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;
        auto value_handle = to_internal(value);
        auto result_handle = to_internal(result);
        result_handle.set(ctx.types().type_of(value_handle));
        return TIRO_OK;
    });
}

tiro_errc
tiro_value_to_string(tiro_vm* vm, tiro_handle value, tiro_handle result, tiro_error** err) {
    if (!vm || !value || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&] {
        vm::Context& ctx = vm->ctx;
        auto value_handle = to_internal(value);
        auto result_handle = to_internal(result);

        vm::Scope sc(ctx);
        vm::Local builder = sc.local(vm::StringBuilder::make(ctx, 32));
        vm::to_string(ctx, builder, value_handle);
        result_handle.set(builder->to_string(ctx));
        return TIRO_OK;
    });
}

tiro_errc tiro_kind_type(tiro_vm* vm, tiro_kind kind, tiro_handle result, tiro_error** err) {
    if (!vm || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;

        auto vm_type = get_type(kind);
        if (!vm_type)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto result_handle = to_internal(result);
        result_handle.set(ctx.types().type_of(*vm_type));
        return TIRO_OK;
    });
}

void tiro_make_null(tiro_vm* vm, tiro_handle result) {
    if (!vm || !result)
        return;

    to_internal(result).set(vm::Value::null());
}

tiro_errc tiro_make_boolean(tiro_vm* vm, bool value, tiro_handle result, tiro_error** err) {
    if (!vm || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;

        auto result_handle = to_internal(result);
        result_handle.set(ctx.get_boolean(value));
        return TIRO_OK;
    });
}

bool tiro_boolean_value(tiro_vm* vm, tiro_handle value) {
    if (!vm)
        return false;

    try {
        vm::Context& ctx = vm->ctx;
        return ctx.is_truthy(to_internal(value));
    } catch (...) {
        return false;
    }
}

tiro_errc tiro_make_integer(tiro_vm* vm, int64_t value, tiro_handle result, tiro_error** err) {
    if (!vm || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;

        auto result_handle = to_internal(result);
        result_handle.set(ctx.get_integer(value));
        return TIRO_OK;
    });
}

int64_t tiro_integer_value(tiro_vm* vm, tiro_handle value) {
    if (!vm || !value)
        return 0;

    auto handle = to_internal(value);
    if (auto i = vm::try_convert_integer(*handle))
        return *i;
    return 0;
}

tiro_errc tiro_make_float(tiro_vm* vm, double value, tiro_handle result, tiro_error** err) {
    if (!vm || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;

        auto result_handle = to_internal(result);
        result_handle.set(vm::Float::make(ctx, value));
        return TIRO_OK;
    });
}

double tiro_float_value(tiro_vm* vm, tiro_handle value) {
    if (!vm || !value)
        return 0;

    auto handle = to_internal(value);
    if (auto f = vm::try_convert_float(*handle))
        return *f;

    return 0;
}

tiro_errc tiro_make_string(tiro_vm* vm, const char* value, tiro_handle result, tiro_error** err) {
    return tiro_make_string_from_data(vm, value, value != NULL ? strlen(value) : 0, result, err);
}

tiro_errc tiro_make_string_from_data(
    tiro_vm* vm, const char* data, size_t length, tiro_handle result, tiro_error** err) {
    if (!vm || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;

        auto result_handle = to_internal(result);
        result_handle.set(vm::String::make(ctx, std::string_view(data, length)));
        return TIRO_OK;
    });
}

tiro_errc tiro_string_value(
    tiro_vm* vm, tiro_handle string, const char** data, size_t* length, tiro_error** err) {
    if (!vm || !string || !data || !length)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        auto maybe_string_handle = to_internal(string).try_cast<vm::String>();
        if (!maybe_string_handle)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto string_handle = maybe_string_handle.handle();
        auto storage = string_handle->view();
        *data = storage.data();
        *length = storage.length();
        return TIRO_OK;
    });
}

tiro_errc tiro_string_cstr(tiro_vm* vm, tiro_handle string, char** result, tiro_error** err) {
    if (!vm || !string || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        auto maybe_string_handle = to_internal(string).try_cast<vm::String>();
        if (!maybe_string_handle)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto string_handle = maybe_string_handle.handle();
        *result = copy_to_cstr(string_handle->view());
        return TIRO_OK;
    });
}

tiro_errc tiro_make_tuple(tiro_vm* vm, size_t size, tiro_handle result, tiro_error** err) {
    if (!vm || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;

        auto result_handle = to_internal(result);
        result_handle.set(vm::Tuple::make(ctx, size));
        return TIRO_OK;
    });
}

size_t tiro_tuple_size(tiro_vm* vm, tiro_handle tuple) {
    if (!vm || !tuple)
        return 0;

    try {
        auto maybe_tuple = to_internal(tuple).try_cast<vm::Tuple>();
        if (!maybe_tuple)
            return 0;

        return maybe_tuple.handle()->size();
    } catch (...) {
        return 0;
    }
}

tiro_errc
tiro_tuple_get(tiro_vm* vm, tiro_handle tuple, size_t index, tiro_handle result, tiro_error** err) {
    if (!vm || !tuple || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        auto maybe_tuple = to_internal(tuple).try_cast<vm::Tuple>();
        if (!maybe_tuple)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto tuple_handle = maybe_tuple.handle();
        if (size_t size = tuple_handle->size(); index >= size)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        to_internal(result).set(tuple_handle->get(index));
        return TIRO_OK;
    });
}

tiro_errc
tiro_tuple_set(tiro_vm* vm, tiro_handle tuple, size_t index, tiro_handle value, tiro_error** err) {
    if (!vm || !tuple || !value)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        auto maybe_tuple = to_internal(tuple).try_cast<vm::Tuple>();
        if (!maybe_tuple)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto tuple_handle = maybe_tuple.handle();
        if (size_t size = tuple_handle->size(); index >= size)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        tuple_handle->set(index, *to_internal(value));
        return TIRO_OK;
    });
}

tiro_errc
tiro_make_array(tiro_vm* vm, size_t initial_capacity, tiro_handle result, tiro_error** err) {
    if (!vm || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;

        auto result_handle = to_internal(result);
        result_handle.set(vm::Array::make(ctx, initial_capacity));
        return TIRO_OK;
    });
}

size_t tiro_array_size(tiro_vm* vm, tiro_handle array) {
    if (!vm || !array)
        return 0;

    try {
        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return 0;

        return maybe_array.handle()->size();
    } catch (...) {
        return 0;
    }
}

tiro_errc
tiro_array_get(tiro_vm* vm, tiro_handle array, size_t index, tiro_handle result, tiro_error** err) {
    if (!vm || !array || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        if (size_t size = array_handle->size(); index >= size)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        to_internal(result).set(array_handle->get(index));
        return TIRO_OK;
    });
}

tiro_errc
tiro_array_set(tiro_vm* vm, tiro_handle array, size_t index, tiro_handle value, tiro_error** err) {
    if (!vm || !array || !value)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        if (size_t size = array_handle->size(); index >= size)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        array_handle->set(index, to_internal(value));
        return TIRO_OK;
    });
}

tiro_errc tiro_array_push(tiro_vm* vm, tiro_handle array, tiro_handle value, tiro_error** err) {
    if (!vm || !array || !value)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;

        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        array_handle->append(ctx, to_internal(value));
        return TIRO_OK;
    });
}

tiro_errc tiro_array_pop(tiro_vm* vm, tiro_handle array, tiro_error** err) {
    if (!vm || !array)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        if (array_handle->size() == 0)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        array_handle->remove_last();
        return TIRO_OK;
    });
}

tiro_errc tiro_array_clear(tiro_vm* vm, tiro_handle array, tiro_error** err) {
    if (!vm || !array)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        auto maybe_array = to_internal(array).try_cast<vm::Array>();
        if (!maybe_array)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto array_handle = maybe_array.handle();
        array_handle->clear();
        return TIRO_OK;
    });
}

tiro_errc tiro_make_coroutine(
    tiro_vm* vm, tiro_handle func, tiro_handle arguments, tiro_handle result, tiro_error** err) {
    if (!vm || !func || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    if (tiro_value_kind(vm, func) != TIRO_KIND_FUNCTION)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

    return api_wrap(err, [&] {
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
        return TIRO_OK;
    });
}

bool tiro_coroutine_started(tiro_vm* vm, tiro_handle coroutine) {
    if (!vm || !coroutine)
        return false;

    try {
        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return false;

        return maybe_coro.handle()->state() != vm::CoroutineState::New;
    } catch (...) {
        return false;
    }
}

bool tiro_coroutine_completed(tiro_vm* vm, tiro_handle coroutine) {
    if (!vm || !coroutine)
        return false;

    try {
        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return false;

        return maybe_coro.handle()->state() == vm::CoroutineState::Done;
    } catch (...) {
        return false;
    }
}

tiro_errc
tiro_coroutine_result(tiro_vm* vm, tiro_handle coroutine, tiro_handle result, tiro_error** err) {
    if (!vm || !coroutine || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&] {
        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto coro_handle = maybe_coro.handle();
        if (coro_handle->state() != vm::CoroutineState::Done)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        auto result_handle = to_internal(result);
        result_handle.set(coro_handle->result());
        return TIRO_OK;
    });
}

tiro_errc
tiro_coroutine_set_callback(tiro_vm* vm, tiro_handle coroutine, tiro_coroutine_callback callback,
    tiro_coroutine_cleanup cleanup, void* userdata, tiro_error** err) {
    if (!vm || !coroutine || !callback)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&] {
        struct Callback : vm::CoroutineCallback {
            tiro_vm* vm;
            tiro_coroutine_callback callback;
            tiro_coroutine_cleanup cleanup;
            void* userdata;

            Callback(tiro_vm* vm_, tiro_coroutine_callback callback_,
                tiro_coroutine_cleanup cleanup_, void* userdata_) noexcept
                : vm(vm_)
                , callback(callback_)
                , cleanup(cleanup_)
                , userdata(userdata_) {}

            Callback(Callback&& other) noexcept
                : vm(std::exchange(other.vm, nullptr))
                , callback(std::exchange(other.callback, nullptr))
                , cleanup(std::exchange(other.cleanup, nullptr))
                , userdata(std::exchange(other.userdata, nullptr)) {}

            ~Callback() {
                if (cleanup)
                    cleanup(vm, userdata);
            }

            void done(vm::Handle<vm::Coroutine> coroutine) override {
                TIRO_DEBUG_ASSERT(callback, "Cannot invoke callback on invalid instance.");

                // Must create a local handle because the external c api only understands
                // mutable values for now. Improvement: const_handles in the api.
                vm::Scope sc(vm->ctx);
                vm::Local coro = sc.local<vm::Value>(coroutine);
                callback(vm, to_external(coro.mut()), userdata);
            }

            void move(void* dest, size_t size) {
                TIRO_DEBUG_ASSERT(dest, "Invalid move destination.");
                TIRO_DEBUG_ASSERT(size == sizeof(*this), "Invalid move destination size.");
                new (dest) Callback(std::move(*this));
            }

            size_t size() override { return sizeof(*this); }

            size_t align() override { return alignof(*this); }
        };

        vm::Context& ctx = vm->ctx;

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto coro_handle = maybe_coro.handle();

        Callback cb(vm, callback, cleanup, userdata);
        ctx.set_callback(coro_handle, cb);
        return TIRO_OK;
    });
}

tiro_errc tiro_coroutine_start(tiro_vm* vm, tiro_handle coroutine, tiro_error** err) {
    if (!vm || !coroutine)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&] {
        vm::Context& ctx = vm->ctx;

        auto maybe_coro = to_internal(coroutine).try_cast<vm::Coroutine>();
        if (!maybe_coro)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto coro_handle = maybe_coro.handle();
        if (coro_handle->state() != vm::CoroutineState::New)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        ctx.start(coro_handle);
        return TIRO_OK;
    });
}

tiro_errc tiro_type_name(tiro_vm* vm, tiro_handle type, tiro_handle result, tiro_error** err) {
    if (!vm || !type || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        auto maybe_type = to_internal(type).try_cast<vm::Type>();
        if (!maybe_type)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto type_handle = maybe_type.handle();
        auto result_handle = to_internal(result);
        result_handle.set(type_handle->name());
        return TIRO_OK;
    });
}
