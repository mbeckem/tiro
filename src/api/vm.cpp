#include "api/internal.hpp"

#include "vm/load_module.hpp"
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

tiro_vm_t tiro_vm_new(const tiro_vm_settings_t* settings, tiro_error_t* err) {
    return entry_point(err, nullptr, [&] {
        auto& actual_settings = settings ? *settings : default_settings;
        return new tiro_vm(actual_settings);
    });
}

void tiro_vm_free(tiro_vm_t vm) {
    delete vm;
}

void* tiro_vm_userdata(tiro_vm_t vm) {
    if (!vm)
        return nullptr;

    return vm->settings.userdata;
}

void tiro_vm_load_std(tiro_vm_t vm, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);
        vm::Local module = sc.local<vm::Module>(vm::defer_init);

        module = vm::create_std_module(ctx);
        if (!ctx.add_module(module))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);
    });
}

void tiro_vm_load_bytecode(tiro_vm_t vm, const tiro_module_t module, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !module || !module->mod)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);
        vm::Local vm_module = sc.local(vm::load_module(ctx, *module->mod));
        if (!ctx.add_module(vm_module))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);
    });
}

void tiro_vm_load_module(tiro_vm_t vm, tiro_handle_t module, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !module)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_module = to_internal(module).try_cast<vm::Module>();
        if (!maybe_module)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto module_handle = maybe_module.handle();
        if (!ctx.add_module(module_handle))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);
    });
}

void tiro_vm_get_export(tiro_vm_t vm, const char* module_name, const char* export_name,
    tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !module_name || !export_name || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);

        // Find the module.
        vm::Local module = sc.local<vm::Module>(vm::defer_init);
        {
            vm::Local vm_name = sc.local(vm::String::make(ctx, module_name));
            if (auto found = ctx.get_module(vm_name)) {
                module.set(*found);
            } else {
                return TIRO_REPORT(err, TIRO_ERROR_MODULE_NOT_FOUND);
            }
        }

        // Find the exported member in the module.
        {
            vm::Local vm_name = sc.local(ctx.get_symbol(export_name));
            if (auto found = module->find_exported(*vm_name)) {
                to_internal(result).set(*found);
            } else {
                return TIRO_REPORT(err, TIRO_ERROR_EXPORT_NOT_FOUND);
            }
        }
    });
}

void tiro_vm_call(tiro_vm_t vm, tiro_handle_t function, tiro_handle_t arguments,
    tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !function)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

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
        if (ret_handle)
            ret_handle.handle().set(retval);
    });
}

void tiro_vm_run_ready(tiro_vm_t vm, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        ctx.run_ready();
    });
}

bool tiro_vm_has_ready(tiro_vm_t vm) {
    return entry_point(nullptr, false, [&] {
        if (!vm)
            return false;

        vm::Context& ctx = vm->ctx;
        return ctx.has_ready();
    });
}

tiro_handle_t tiro_global_new(tiro_vm_t vm, tiro_error_t* err) {
    return entry_point(err, nullptr, [&]() -> tiro_handle_t {
        if (!vm)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG), nullptr;

        vm::Context& ctx = vm->ctx;
        auto external = ctx.externals().allocate();
        return to_external(external.mut());
    });
}

void tiro_global_free(tiro_vm_t vm, tiro_handle_t global) {
    return entry_point(nullptr, [&] {
        if (!vm || !global)
            return;

        vm::Context& ctx = vm->ctx;
        auto global_handle = to_internal(global);
        auto external = vm::External<vm::Value>::from_raw_slot(vm::get_valid_slot(global_handle));
        ctx.externals().free(external);
    });
}
