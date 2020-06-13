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
        vm::Root<vm::Module> module(vm->ctx);

        module.set(vm::create_std_module(vm->ctx));
        if (!vm->ctx.add_module(module))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);

        module.set(vm::create_io_module(vm->ctx));
        if (!vm->ctx.add_module(module))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);

        return TIRO_OK;
    });
}

tiro_errc tiro_vm_load(tiro_vm* vm, const tiro_module* module, tiro_error** err) {
    if (!vm || !module || !module->mod)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Root<vm::Module> vm_module(vm->ctx, vm::load_module(vm->ctx, *module->mod));
        if (!vm->ctx.add_module(vm_module))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);

        return TIRO_OK;
    });
}

tiro_errc tiro_vm_run(tiro_vm* vm, const char* module_name, const char* function_name,
    char** result, tiro_error** err) {
    if (!vm || !module_name || !function_name || !result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        std::string_view mname = module_name;
        std::string_view fname = function_name;

        vm::Context& ctx = vm->ctx;

        // Find the module.
        vm::Root<vm::Module> module(ctx);
        {
            vm::Root<vm::String> vm_name(ctx, vm::String::make(ctx, mname));
            if (!ctx.find_module(vm_name, module.mut_handle()))
                return TIRO_REPORT(err, TIRO_ERROR_MODULE_NOT_FOUND);
        }

        // Find the function in the module.
        vm::Root<vm::Function> function(ctx);
        {
            vm::Root<vm::Tuple> members(ctx, module->members());

            // TODO: Support for exported entities
            for (size_t i = 0, n = members->size(); i < n; ++i) {
                vm::Value v = members->get(i);
                if (v.is<vm::Function>()) {
                    vm::Function f = v.as<vm::Function>();
                    if (f.tmpl().name().view() == fname)
                        function.set(f);
                }
            }
            if (function->is_null())
                return TIRO_REPORT(err, TIRO_ERROR_FUNCTION_NOT_FOUND);
        }

        vm::Root<vm::Value> return_value(ctx, ctx.run(function.handle(), {}));
        *result = copy_to_cstr(to_string(return_value));
        return TIRO_OK;
    });
}
