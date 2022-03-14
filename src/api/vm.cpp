#include "api/internal.hpp"

#include "vm/builtins/modules.hpp"
#include "vm/heap/chunks.hpp"
#include "vm/modules/load.hpp"
#include "vm/modules/registry.hpp"
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
        auto& raw_settings = settings ? *settings : default_settings;
        tiro::vm::ContextSettings internal_settings;

        if (auto page_size = raw_settings.page_size) // 0 -> leave at default value
            internal_settings.page_size_bytes = page_size;

        if (auto max_heap_size = raw_settings.max_heap_size) // 0 -> leave at default value
            internal_settings.max_heap_size_bytes = max_heap_size;

        if (raw_settings.print_stdout) {
            auto func = raw_settings.print_stdout;
            auto userdata = raw_settings.userdata;
            internal_settings.print_stdout = [func, userdata](std::string_view message) {
                func(to_external(message), userdata);
            };
        }

        internal_settings.enable_panic_stack_traces = raw_settings.enable_panic_stack_trace;

        return new tiro_vm(raw_settings.userdata, std::move(internal_settings));
    });
}

void tiro_vm_free(tiro_vm_t vm) {
    delete vm;
}

void* tiro_vm_userdata(tiro_vm_t vm) {
    if (!vm)
        return nullptr;

    return vm->external_userdata;
}

size_t tiro_vm_page_size(tiro_vm_t vm) {
    if (!vm)
        return 0;
    return vm->ctx.heap().layout().page_size;
}

size_t tiro_vm_max_heap_size(tiro_vm_t vm) {
    if (!vm)
        return 0;
    return vm->ctx.heap().max_size();
}

void tiro_vm_load_std(tiro_vm_t vm, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);
        vm::Local module = sc.local<vm::Module>(vm::defer_init);

        module = vm::create_std_module(ctx);
        if (!ctx.modules().add_module(ctx, module))
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
        if (!ctx.modules().add_module(ctx, vm_module))
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
        if (!ctx.modules().add_module(ctx, module_handle))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);
    });
}

void tiro_vm_get_export(tiro_vm_t vm, tiro_string_t module_name, tiro_string_t export_name,
    tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !result || !valid_string(module_name) || !valid_string(export_name)
            || module_name.length == 0 || export_name.length == 0)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;
        vm::Scope sc(ctx);

        // Find the module.
        vm::Local module = sc.local<vm::Module>(vm::defer_init);
        {
            vm::Local vm_name = sc.local(vm::String::make(ctx, to_internal(module_name)));
            if (auto found = ctx.modules().get_module(ctx, vm_name)) {
                module.set(*found);
            } else {
                return TIRO_REPORT(err, TIRO_ERROR_MODULE_NOT_FOUND);
            }
        }

        // Find the exported member in the module.
        {
            vm::Local vm_name = sc.local(ctx.get_symbol(to_internal(export_name)));
            if (auto found = module->find_exported(*vm_name)) {
                to_internal(result).set(*found);
            } else {
                return TIRO_REPORT(err, TIRO_ERROR_EXPORT_NOT_FOUND);
            }
        }
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

void tiro_global_free(tiro_handle_t global) {
    return entry_point(nullptr, [&] {
        if (!global)
            return;

        auto global_handle = to_internal(global);
        auto external = vm::External<vm::Value>::from_raw_slot(vm::get_valid_slot(global_handle));
        auto storage = vm::ExternalStorage::from_external(external);
        storage->free(external);
    });
}
