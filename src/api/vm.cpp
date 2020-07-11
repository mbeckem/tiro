#include "api/private.hpp"

#include "vm/load.hpp"
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
