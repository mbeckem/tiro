#include "tiro/api.h"

#include "tiro/bytecode/module.hpp"
#include "tiro/compiler/compiler.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/math.hpp"
#include "tiro/heap/handles.hpp"
#include "tiro/modules/modules.hpp"
#include "tiro/vm/context.hpp"
#include "tiro/vm/load.hpp"

#include <exception>
#include <stdexcept>

using namespace tiro;

// Can't use base class with virtual functions - forbidden in constexpr context.
struct tiro_error_vtable {
    void (*destroy)(tiro_error* self) = nullptr;
    const char* (*details)(const tiro_error* self) = nullptr;
};

struct tiro_error {
    const tiro_error_vtable* vtable = nullptr;
    tiro_errc errc = TIRO_ERROR_INTERNAL;
    SourceLocation source;
};

struct tiro_static_error final : tiro_error {};

static constexpr tiro_error_vtable static_error_vtable = []() {
    tiro_error_vtable vtable{};
    return vtable;
}();

static constexpr tiro_static_error
build_static_error(tiro_errc errc, const SourceLocation& source) {
    tiro_static_error error{};
    error.vtable = &static_error_vtable;
    error.errc = errc;
    error.source = source;
    return error;
}

static constexpr tiro_static_error internal_error = build_static_error(
    TIRO_ERROR_INTERNAL, TIRO_SOURCE_LOCATION());

static constexpr tiro_static_error alloc_error = build_static_error(
    TIRO_ERROR_ALLOC, TIRO_SOURCE_LOCATION());

struct tiro_dynamic_error final : tiro_error {
    std::string details;
};

static constexpr tiro_error_vtable dynamic_error_vtable = []() {
    tiro_error_vtable vtable;
    vtable.destroy = [](tiro_error* self) {
        delete static_cast<tiro_dynamic_error*>(self);
    };
    vtable.details = [](const tiro_error* self) {
        return static_cast<const tiro_dynamic_error*>(self)->details.c_str();
    };
    return vtable;
}();

struct tiro_vm {
    vm::Context ctx;
    tiro_vm_settings settings;

    explicit tiro_vm(const tiro_vm_settings& settings_)
        : settings(settings_) {}

    tiro_vm(const tiro_vm&) = delete;
    tiro_vm& operator=(const tiro_vm&) = delete;
};

struct tiro_compiler {
    tiro_compiler_settings settings;
    std::optional<Compiler> compiler;
    std::optional<CompilerResult> result;

    explicit tiro_compiler(const tiro_compiler_settings& settings_)
        : settings(settings_) {}

    tiro_compiler(const tiro_compiler&) = delete;
    tiro_compiler& operator=(const tiro_compiler&) = delete;

    void report(const Diagnostics::Message& message);
};

struct tiro_module {
    std::unique_ptr<BytecodeModule> mod;

    tiro_module(std::unique_ptr<BytecodeModule>&& mod_) noexcept
        : mod(std::move(mod_)) {}

    tiro_module(const tiro_module&) = delete;
    tiro_module& operator=(const tiro_module&) = delete;
};

void tiro_compiler::report(const Diagnostics::Message& message) {
    if (!settings.message_callback || !compiler)
        return;

    const tiro_severity severity = [&]() {
        switch (message.level) {
        case Diagnostics::Error:
            return TIRO_SEVERITY_ERROR;
        case Diagnostics::Warning:
            return TIRO_SEVERITY_WARNING;
        }

        TIRO_UNREACHABLE("Invalid diagnostic level.");
        return TIRO_SEVERITY_ERROR;
    }();

    CursorPosition pos = compiler->cursor_pos(message.source);
    settings.message_callback(severity, pos.line(), pos.column(),
        message.text.c_str(), settings.message_callback_data);
}

static constexpr tiro_vm_settings default_settings = []() {
    tiro_vm_settings settings{};
    return settings;
}();

static constexpr tiro_compiler_settings default_compiler_settings = []() {
    tiro_compiler_settings settings{};
    settings.message_callback = [](tiro_severity severity, uint32_t line,
                                    uint32_t column, const char* message,
                                    void*) {
        try {
            FILE* out = stdout;
            fmt::print(out, "{} [{}:{}]: {}\n", tiro_severity_str(severity),
                line, column, message);
            std::fflush(out);
        } catch (...) {
        }
    };
    return settings;
}();

static char* to_cstr(const std::string_view str) {
    const size_t string_size = str.size();

    size_t alloc_size = string_size;
    if (!checked_add<size_t>(alloc_size, 1))
        throw std::bad_alloc();

    char* result = static_cast<char*>(::malloc(alloc_size));
    if (!result)
        throw std::bad_alloc();

    std::memcpy(result, str.data(), string_size);
    result[string_size] = '\0';
    return result;
}

#define TIRO_REPORT(err, ...) \
    report_error((err), TIRO_SOURCE_LOCATION(), __VA_ARGS__)

[[nodiscard]] static tiro_errc
report_static_error(tiro_error** err, const tiro_static_error& error) {
    // Casting the const away is safe because errors do not have a mutable public interface.
    if (err) {
        *err = static_cast<tiro_error*>(const_cast<tiro_static_error*>(&error));
    }
    return error.errc;
}

[[nodiscard]] static tiro_errc
report_error(tiro_error** err, const SourceLocation& source, tiro_errc errc,
    FunctionRef<std::string()> produce_details = {}) {
    if (!err || *err) // Do not overwrite existing errors.
        return errc;

    try {
        auto instance = std::make_unique<tiro_dynamic_error>();
        instance->vtable = &dynamic_error_vtable;
        instance->errc = errc;
        instance->source = source;
        if (produce_details)
            instance->details = produce_details();

        *err = instance.release();
        return errc;
    } catch (...) {
        return report_static_error(err, internal_error);
    }
}

[[nodiscard]] static tiro_errc report_exception(tiro_error** err) {
    auto ptr = std::current_exception();
    try {
        std::rethrow_exception(ptr);
    } catch (const Error& ex) {
        // TODO: tiro exceptions should have file/line in debug mode
        return report_error(
            err, {}, TIRO_ERROR_INTERNAL, [&]() { return ex.what(); });
    } catch (const std::bad_alloc& ex) {
        return report_static_error(err, alloc_error);
    } catch (const std::exception& ex) {
        return report_error(
            err, {}, TIRO_ERROR_INTERNAL, [&]() { return ex.what(); });
    } catch (...) {
        return report_error(err, {}, TIRO_ERROR_INTERNAL,
            [&]() { return "Exception of unknown type."; });
    }
    return TIRO_OK;
}

// Eats all exceptions. This is necessary because we're being called by C code.
template<typename ApiFunc>
[[nodiscard]] static tiro_errc
api_wrap(tiro_error** err, ApiFunc&& fn) noexcept {
    try {
        if constexpr (std::is_same_v<decltype(fn()), void>) {
            fn();
            return TIRO_OK;
        } else {
            return fn();
        }
    } catch (...) {
        return report_exception(err);
    }
}

const char* tiro_errc_name(tiro_errc e) {
    switch (e) {
#define TIRO_ERRC_NAME(X) \
    case TIRO_##X:        \
        return #X;

        TIRO_ERRC_NAME(OK)
        TIRO_ERRC_NAME(ERROR_BAD_STATE)
        TIRO_ERRC_NAME(ERROR_BAD_ARG)
        TIRO_ERRC_NAME(ERROR_BAD_SOURCE)
        TIRO_ERRC_NAME(ERROR_MODULE_EXISTS)
        TIRO_ERRC_NAME(ERROR_MODULE_NOT_FOUND)
        TIRO_ERRC_NAME(ERROR_FUNCTION_NOT_FOUND)
        TIRO_ERRC_NAME(ERROR_ALLOC)
        TIRO_ERRC_NAME(ERROR_INTERNAL)

#undef TIRO_ERRC_NAME
    }
    return "<INVALID ERROR CODE>";
}

const char* tiro_errc_message(tiro_errc e) {
    switch (e) {
#define TIRO_ERRC_MESSAGE(X, str) \
    case TIRO_##X:                \
        return str;

        TIRO_ERRC_MESSAGE(OK, "No error.")
        TIRO_ERRC_MESSAGE(ERROR_BAD_STATE,
            "The instance is not in a valid state for this operation.");
        TIRO_ERRC_MESSAGE(ERROR_BAD_ARG, "Invalid argument.")
        TIRO_ERRC_MESSAGE(ERROR_BAD_SOURCE, "The source code contains errors.")
        TIRO_ERRC_MESSAGE(
            ERROR_MODULE_EXISTS, "A module with that name already exists.")
        TIRO_ERRC_MESSAGE(ERROR_MODULE_NOT_FOUND,
            "The requested module is unknown to the vm.")
        TIRO_ERRC_MESSAGE(ERROR_FUNCTION_NOT_FOUND,
            "The requested function is unknown to the vm.")
        TIRO_ERRC_MESSAGE(ERROR_ALLOC, "Object allocation failed.")
        TIRO_ERRC_MESSAGE(ERROR_INTERNAL, "An internal error occurred.")
    }
    return "<INVALID ERROR CODE>";
}

void tiro_error_free(tiro_error* err) {
    if (err && err->vtable && err->vtable->destroy)
        err->vtable->destroy(err);
}

tiro_errc tiro_error_errc(const tiro_error* err) {
    return err ? err->errc : TIRO_OK;
}

const char* tiro_error_name(const tiro_error* err) {
    return tiro_errc_name(tiro_error_errc(err));
}

const char* tiro_error_message(const tiro_error* err) {
    return tiro_errc_message(tiro_error_errc(err));
}

const char* tiro_error_details(const tiro_error* err) {
    return err && err->vtable && err->vtable->details
               ? err->vtable->details(err)
               : "";
}

const char* tiro_error_file(const tiro_error* err) {
    if (!err || !err->source.file)
        return "";
    return err->source.file;
}

int tiro_error_line(const tiro_error* err) {
    return err ? err->source.line : 0;
}

const char* tiro_error_func(const tiro_error* err) {
    if (!err || !err->source.function)
        return "";
    return err->source.function;
}

const char* tiro_severity_str(tiro_severity severity) {
    switch (severity) {
    case TIRO_SEVERITY_WARNING:
        return "WARNING";
    case TIRO_SEVERITY_ERROR:
        return "ERROR";
    }
    return "<INVALID SEVERITY>";
}

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

tiro_errc
tiro_vm_load(tiro_vm* vm, const tiro_module* module, tiro_error** err) {
    if (!vm || !module || !module->mod)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        vm::Root<vm::Module> vm_module(
            vm->ctx, vm::load_module(vm->ctx, *module->mod));
        if (!vm->ctx.add_module(vm_module))
            return TIRO_REPORT(err, TIRO_ERROR_MODULE_EXISTS);

        return TIRO_OK;
    });
}

tiro_errc tiro_vm_run(tiro_vm* vm, const char* module_name,
    const char* function_name, char** result, tiro_error** err) {
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
        *result = to_cstr(to_string(return_value));
        return TIRO_OK;
    });
}

void tiro_compiler_settings_init(tiro_compiler_settings* settings) {
    if (!settings)
        return;

    *settings = default_compiler_settings;
}

tiro_compiler* tiro_compiler_new(const tiro_compiler_settings* settings) {
    try {
        return new tiro_compiler(
            settings ? *settings : default_compiler_settings);
    } catch (...) {
        return nullptr;
    }
}

void tiro_compiler_free(tiro_compiler* compiler) {
    delete compiler;
}

tiro_errc tiro_compiler_add_file(tiro_compiler* comp, const char* file_name,
    const char* file_content, tiro_error** err) {
    if (!comp || !file_name || !file_content)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    std::string_view file_name_view = file_name;
    std::string_view file_content_view = file_content;
    if (file_name_view.empty())
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    if (comp->compiler) // TODO: Only for as long as constructor requires file name and content
        return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

    return api_wrap(err, [&]() {
        CompilerOptions options;
        options.analyze = options.parse = options.compile = true;
        options.keep_ast = comp->settings.enable_dump_ast;
        options.keep_ir = comp->settings.enable_dump_ir;
        options.keep_bytecode = comp->settings.enable_dump_bytecode;
        comp->compiler.emplace(file_name_view, file_content_view, options);
    });
}

tiro_errc tiro_compiler_run(tiro_compiler* comp, tiro_error** err) {
    if (!comp)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    if (!comp->compiler || comp->result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

    return api_wrap(err, [&]() {
        Compiler& compiler = *comp->compiler;

        comp->result = compiler.run();
        for (const auto& message : compiler.diag().messages()) {
            comp->report(message);
        }

        if (!comp->result->success)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_SOURCE);

        return TIRO_OK;
    });
}

bool tiro_compiler_has_module(tiro_compiler* comp) {
    return comp && comp->result && comp->result->module;
}

tiro_errc tiro_compiler_take_module(
    tiro_compiler* comp, tiro_module** module, tiro_error** err) {
    if (!comp || !module)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    if (!tiro_compiler_has_module(comp))
        return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

    return api_wrap(err, [&]() {
        auto& compiled = comp->result->module;

        auto result = std::make_unique<tiro_module>(std::move(compiled));
        *module = result.release();
        return TIRO_OK;
    });
}

tiro_errc
tiro_compiler_dump_ast(tiro_compiler* comp, char** string, tiro_error** err) {
    if (!comp || !string)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        if (!comp->result || !comp->result->ast)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        *string = to_cstr(*comp->result->ast);
        return TIRO_OK;
    });
}

tiro_errc
tiro_compiler_dump_ir(tiro_compiler* comp, char** string, tiro_error** err) {
    if (!comp || !string)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        if (!comp->result || !comp->result->ir)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        *string = to_cstr(*comp->result->ir);
        return TIRO_OK;
    });
}

tiro_errc tiro_compiler_dump_bytecode(
    tiro_compiler* comp, char** string, tiro_error** err) {
    if (!comp || !string)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        if (!comp->result || !comp->result->bytecode)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        *string = to_cstr(*comp->result->bytecode);
        return TIRO_OK;
    });
}

void tiro_module_free(tiro_module* module) {
    delete module;
}
