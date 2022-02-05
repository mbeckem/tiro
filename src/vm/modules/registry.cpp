#include "vm/modules/registry.hpp"

#include "bytecode/module.hpp"
#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/math.hpp"
#include "vm/objects/exception.hpp"
#include "vm/objects/module.hpp"
#include "vm/objects/result.hpp"
#include "vm/objects/string.hpp"
#include "vm/objects/tuple.hpp"

// #define TIRO_TRACE_RESOLUTION

namespace tiro::vm {

void ModuleRegistry::init(Context& ctx) {
    modules_ = HashTable::make(ctx, 64).must("failed to allocate module index");
}

bool ModuleRegistry::add_module(Context& ctx, Handle<Module> module) {
    if (modules_.value().contains(module->name())) {
        return false;
    }

    Scope sc(ctx);
    Local name = sc.local(module->name());
    name = ctx.get_interned_string(name);
    modules_.value().set(ctx, name, module).must("failed to add module to index");
    return true;
}

std::optional<Module> ModuleRegistry::get_module(Context& ctx, Handle<String> module_name) {
    Scope sc(ctx);
    Local module = sc.local<Module>(defer_init);
    if (auto found = find_module(*module_name)) {
        module = *found;
    } else {
        return {};
    }

    resolve_module(ctx, module);
    return *module;
}

void ModuleRegistry::resolve_module(Context& ctx, Handle<Module> module) {
    if (module->initialized())
        return;

    enum State { Enter, Dependencies, Init, Exit };

    struct Frame {
        State state_ = Enter;
        UniqueExternal<Module> module_;
        size_t next_member_ = 0;
        size_t total_members_ = 0;

        Frame(ExternalStorage& storage, Handle<Module> module)
            : module_(storage.allocate(module))
            , total_members_(module->members().size()) {}
    };

    Scope sc(ctx);
    Local active = sc.local(
        HashTable::make(ctx, 16).must("failed to allocate import cycle detection set"));

    // TODO: Reuse stack space?
    std::vector<Frame> stack;

    // Visits a module that is being referenced from an importing module. If the referenced module
    // is not already initialized, a new frame is pushed to the stack to begin the initialization.
    // This *INVALIDATES THE CURRENT FRAME REFERENCE*.
    auto recurse = [&](Handle<Module> m) {
        if (m->initialized())
            return false;

        if (stack.size() >= 2048) {
            TIRO_ERROR(
                "module resolution recursion limit reached, imports are nested too deep (depth {})",
                stack.size());
        }

        stack.emplace_back(ctx.externals(), m);
        return true;
    };

    auto on_import_cycle = [&](size_t current_index, size_t original_index) {
        TIRO_DEBUG_ASSERT(current_index > original_index,
            "index of invalid cyclic import must be greater than the original index");
        TIRO_DEBUG_ASSERT(current_index < stack.size(), "index out of bounds.");

        StringFormatStream message;
        message.format("module {} is part of a forbidden dependency cycle:\n",
            stack[current_index].module_->name().view());

        for (size_t i = original_index; i <= current_index; ++i) {
            message.format("- {}: module {}", i - original_index, stack[i].module_->name().view());
            if (i != current_index)
                message.format(", imports\n");
        }

        TIRO_ERROR("{}", message.take_str());
    };

    if (!recurse(module))
        return;

    Local current_name = sc.local<String>(defer_init);
    Local current_index = sc.local();
    Local current_members = sc.local<Tuple>(defer_init);
    Local current_member = sc.local();
    Local current_init = sc.local();
    Local init_result = sc.local<Result>(defer_init);
    Local imported_name = sc.local<String>(defer_init);
    Local imported_module = sc.local<Module>(defer_init);
    while (!stack.empty()) {
        auto& frame = stack.back();
        TIRO_DEBUG_ASSERT(!frame.module_->initialized(), "module must not be initialized already");

    dispatch:
        switch (frame.state_) {

        // Register that this module is currently initializing (cycle detection).
        case Enter: {
#ifdef TIRO_TRACE_RESOLUTION
            fmt::print("> {}: {}\n", stack.size() - 1, frame.module_->name().view());
#endif

            current_name = frame.module_->name();
            if (auto found = active->get(*current_name)) {
                on_import_cycle(stack.size() - 1, found->must_cast<Integer>().value());
            }

            current_index = ctx.get_integer(stack.size() - 1);
            active->set(ctx, current_name, current_index)
                .must("failed to add entry to import cycle detection set");

            frame.state_ = Dependencies;
            goto dispatch;
        }

        // Iterate over all pending module members, resolving imports if necessary. Resolving an import
        // may make recursion necessary, in which case a frame is pushed and execution within the current
        // frame is paused.
        case Dependencies: {
            size_t& i = frame.next_member_;
            size_t n = frame.total_members_;
            if (i < n) {
                current_members = frame.module_->members();
                while (i < n) {
                    current_member = current_members->unchecked_get(i);
                    if (!current_member->is<UnresolvedImport>()) {
                        ++i;
                        continue;
                    }

                    // Search for the imported module and link it into the members tuple on success.
                    imported_name = current_member.must_cast<UnresolvedImport>()->module_name();
                    if (auto found = find_module(*imported_name)) {
                        imported_module = *found;
                    } else {
                        TIRO_ERROR_WITH_CODE(TIRO_ERROR_MODULE_NOT_FOUND,
                            "module '{}' was not found", imported_name->view());
                    }
                    current_members->unchecked_set(i, *imported_module);
                    ++i;

                    // Recurse if necessary. CAREFUL: if recurse() pushes a new frame,
                    // the `frame` is invalidated!
                    if (recurse(imported_module)) {
                        goto next;
                    }
                }
            }
            frame.state_ = Init;
            goto dispatch;
        }

        // All module members have been resolved. Call the module initializer.
        case Init: {
            current_init = frame.module_->initializer();
            if (!current_init->is_null()) {
                init_result = ctx.run_init(current_init, {});
                if (init_result->is_error()) {
                    TIRO_ERROR("module initialization of '{}' failed: {}",
                        frame.module_->name().view(), to_string(init_result->unchecked_error()));
                }
            }
            frame.module_->initialized(true);
            frame.state_ = Exit;
            goto dispatch;
        }

        // Module resolution complete.
        case Exit: {
#ifdef TIRO_TRACE_RESOLUTION
            fmt::print("< {}: {}\n", stack.size() - 1, frame.module_->name().view());
#endif
            [[maybe_unused]] bool removed = active->remove(frame.module_->name());
            TIRO_DEBUG_ASSERT(removed, "module must be registered while it is being initialized");

            stack.pop_back(); // frame invalidated
            goto next;
        }
        }

    next:;
    }
}

std::optional<Module> ModuleRegistry::find_module(String name) {
    if (auto found = modules_.value().get(name))
        return found->must_cast<Module>();
    return {};
}

} // namespace tiro::vm
