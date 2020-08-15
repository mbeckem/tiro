#include "api/internal.hpp"

#include "vm/load.hpp"
#include "vm/modules/modules.hpp"
#include "vm/objects/all.hpp"

#include <new>

using namespace tiro;
using namespace tiro::api;

static constexpr tiro_vm_settings_t default_settings = []() {
    tiro_vm_settings_t settings{};
    return settings;
}();

void tiro_vm_settings_init(tiro_vm_settings_t* settings) {
    if (!settings) {
        return;
    }

    *settings = default_settings;
}

tiro_vm_t tiro_vm_new(const tiro_vm_settings_t* settings) {
    try {
        return new tiro_vm(settings ? *settings : default_settings);
    } catch (...) {
        return nullptr;
    }
}

void tiro_vm_free(tiro_vm_t vm) {
    delete vm;
}

tiro_errc_t tiro_vm_load_std(tiro_vm_t vm, tiro_error_t* err) {
    if (!vm)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);
        vm::Local module = sc.local<vm::Module>(vm::defer_init);

        module = vm::create_std_module(ctx);
        if (!ctx.add_module(module))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);

        return TIRO_OK;
    });
}

tiro_errc_t tiro_vm_load(tiro_vm_t vm, const tiro_module_t module, tiro_error_t* err) {
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

tiro_errc_t tiro_vm_find_function(tiro_vm_t vm, const char* module_name, const char* function_name,
    tiro_handle_t result, tiro_error_t* err) {
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

tiro_errc_t tiro_vm_call(tiro_vm_t vm, tiro_handle_t function, tiro_handle_t arguments,
    tiro_handle_t result, tiro_error_t* err) {
    if (!vm || !function)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&] {
        vm::Context& ctx = vm->ctx;

        auto func_handle = to_internal(function);
        auto arg_handle = to_internal_maybe(arguments);
        auto ret_handle = to_internal_maybe(result);

        if (arg_handle) {
            auto args = arg_handle.handle();
            if (!args->is<vm::Null>() && !args->is<vm::Tuple>())
                return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);
        }

        auto retval = ctx.run_init(func_handle, arg_handle.try_cast<vm::Tuple>());
        if (ret_handle) {
            ret_handle.handle().set(retval);
        }
        return TIRO_OK;
    });
}

tiro_errc_t tiro_vm_run_ready(tiro_vm_t vm, tiro_error_t* err) {
    if (!vm)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&] {
        vm::Context& ctx = vm->ctx;
        ctx.run_ready();
    });
}

bool tiro_vm_has_ready(tiro_vm_t vm) {
    if (!vm)
        return false;

    try {
        vm::Context& ctx = vm->ctx;
        return ctx.has_ready();
    } catch (...) {
        return false;
    }
}

tiro_frame_t tiro_frame_new(tiro_vm_t vm, size_t slots) {
    if (!vm)
        return nullptr;

    try {
        auto internal = vm->ctx.frames().allocate_frame(slots);
        return to_external(internal);
    } catch (...) {
        return nullptr;
    }
}

void tiro_frame_free(tiro_frame_t frame) {
    if (!frame)
        return;

    auto internal = to_internal(frame);
    internal->destroy();
}

size_t tiro_frame_size(tiro_frame_t frame) {
    if (!frame)
        return 0;

    auto internal = to_internal(frame);
    return internal->size();
}

tiro_handle_t tiro_frame_slot(tiro_frame_t frame, size_t slot_index) {
    if (!frame)
        return nullptr;

    auto internal = to_internal(frame);
    if (slot_index >= internal->size())
        return nullptr;

    auto handle = vm::MutHandle<vm::Value>::from_raw_slot(internal->slot(slot_index));
    return to_external(handle);
}
