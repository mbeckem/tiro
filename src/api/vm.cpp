#include "api/internal.hpp"

#include "vm/load.hpp"
#include "vm/math.hpp"
#include "vm/modules/modules.hpp"

using namespace tiro;
using namespace tiro::api;

static constexpr tiro_vm_settings default_settings = []() {
    tiro_vm_settings settings{};
    return settings;
}();

void tiro_vm_settings_init(tiro_vm_settings* settings) {
    if (!settings) {
        return;
    }

    *settings = default_settings;
}

tiro_vm* tiro_vm_new(const tiro_vm_settings* settings) {
    try {
        return new tiro_vm(settings ? *settings : default_settings);
    } catch (...) {
        return nullptr;
    }
}

void tiro_vm_free(tiro_vm* vm) {
    delete vm;
}

tiro_errc tiro_vm_load_std(tiro_vm* vm, tiro_error** err) {
    if (!vm)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);
        vm::Local module = sc.local<vm::Module>(vm::defer_init);

        module = vm::create_std_module(ctx);
        if (!ctx.add_module(module))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);

        module = vm::create_io_module(ctx);
        if (!ctx.add_module(module))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);

        return TIRO_OK;
    });
}

tiro_errc tiro_vm_load(tiro_vm* vm, const tiro_module* module, tiro_error** err) {
    if (!vm || !module || !module->mod)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);
        vm::Local vm_module = sc.local(vm::load_module(ctx, *module->mod));
        if (!ctx.add_module(vm_module))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);

        return TIRO_OK;
    });
}

tiro_errc tiro_vm_run(tiro_vm* vm, const char* module_name, const char* function_name,
    char** result, tiro_error** err) {
    if (!vm || !module_name || !function_name || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);

        // Find the module.
        vm::Local module = sc.local<vm::Module>(vm::defer_init);
        {
            vm::Local vm_name = sc.local(vm::String::make(ctx, module_name));
            if (!ctx.find_module(vm_name, module.out()))
                return TIRO_REPORT(err, TIRO_ERROR_MODULE_NOT_FOUND);
        }

        // Find the function in the module.
        vm::Local function = sc.local();
        {
            vm::Local vm_name = sc.local(ctx.get_symbol(function_name));
            if (auto found = module->find_exported(vm_name)) {
                function.set(*found);
            } else {
                return TIRO_REPORT(err, TIRO_ERROR_FUNCTION_NOT_FOUND);
            }
        }

        vm::Local return_value = sc.local(ctx.run(function, {}));
        *result = copy_to_cstr(to_string(*return_value));
        return TIRO_OK;
    });
}

tiro_errc tiro_vm_find_function(tiro_vm* vm, const char* module_name, const char* function_name,
    tiro_handle result, tiro_error** err) {
    if (!vm || !module_name || !function_name || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);

        // Find the module.
        vm::Local module = sc.local<vm::Module>(vm::defer_init);
        {
            vm::Local vm_name = sc.local(vm::String::make(ctx, module_name));
            if (!ctx.find_module(vm_name, module.out()))
                return TIRO_REPORT(err, TIRO_ERROR_MODULE_NOT_FOUND);
        }

        // Find the function in the module.
        {
            vm::Local vm_name = sc.local(ctx.get_symbol(function_name));
            if (auto found = module->find_exported(vm_name)) {
                to_internal(result).set(*found);
                return TIRO_OK;
            } else {
                return TIRO_REPORT(err, TIRO_ERROR_FUNCTION_NOT_FOUND);
            }
        }
    });
}

tiro_errc tiro_vm_call(tiro_vm* vm, tiro_handle function, tiro_handle arguments, tiro_handle result,
    tiro_error** err) {
    if (!vm || !function)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;

        auto func_handle = to_internal(function);
        auto arg_handle = to_internal_maybe(arguments);
        auto ret_handle = to_internal_maybe(result);

        if (arg_handle) {
            auto args = arg_handle.handle();
            if (!args->is<vm::Null>() && !args->is<vm::Tuple>())
                return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);
        }

        auto retval = ctx.run(func_handle, arg_handle.try_cast<vm::Tuple>());
        if (ret_handle) {
            ret_handle.handle().set(retval);
        }
        return TIRO_OK;
    });
}

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
        TIRO_CASE(TUPLE)
        TIRO_CASE(FUNCTION)
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

    default:
        return TIRO_KIND_INTERNAL;

#undef TIRO_MAP
    }
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
        auto result_handle = to_internal(result);

        auto maybe_tuple = to_internal(tuple).try_cast<vm::Tuple>();
        if (!maybe_tuple)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto tuple_handle = maybe_tuple.handle();
        if (size_t size = tuple_handle->size(); index >= size)
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        result_handle.set(tuple_handle->get(index));
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

tiro_frame* tiro_frame_new(tiro_vm* vm, size_t slots) {
    if (!vm)
        return nullptr;

    try {
        auto internal = vm->ctx.frames().allocate_frame(slots);
        return to_external(internal);
    } catch (...) {
        return nullptr;
    }
}

void tiro_frame_free(tiro_frame* frame) {
    if (!frame)
        return;

    auto internal = to_internal(frame);
    internal->destroy();
}

size_t tiro_frame_size(tiro_frame* frame) {
    if (!frame)
        return 0;

    auto internal = to_internal(frame);
    return internal->size();
}

tiro_handle tiro_frame_slot(tiro_frame* frame, size_t slot_index) {
    if (!frame)
        return nullptr;

    auto internal = to_internal(frame);
    if (slot_index >= internal->size())
        return nullptr;

    auto handle = vm::MutHandle<vm::Value>::from_raw_slot(internal->slot(slot_index));
    return to_external(handle);
}
