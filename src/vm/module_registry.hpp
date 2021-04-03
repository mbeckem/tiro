#ifndef TIRO_VM_MODULE_REGISTRY_HPP
#define TIRO_VM_MODULE_REGISTRY_HPP

#include "bytecode/fwd.hpp"
#include "vm/fwd.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/module.hpp"

#include <optional>

namespace tiro::vm {

class ModuleRegistry final {
public:
    void init(Context& ctx);

    /// Attempts to register the given module with this registry.
    /// Fails (i.e. returns false) if a module with that name has already been registered.
    bool add_module(Context& ctx, Handle<Module> module);

    /// Attempts to find the module with the given name. Modules returned by a successful call
    /// to this function will always be initialized.
    /// Fails (i.e. returns an empty optional) if the module does not exist.
    /// NOTE: Throws if the module exists, but any of its dependencies do not exist.
    std::optional<Module> get_module(Context& ctx, Handle<String> name);

    /// Initializes the module. This includes resolving all imports and invoking the init function if
    /// the module wasn't initialized already.
    /// NOTE: The module itself is *not* registered with the registry.
    void resolve_module(Context& ctx, Handle<Module> module);

    template<typename Tracer>
    inline void trace(Tracer&& tracer);

private:
    std::optional<Module> find_module(String name);

private:
    Nullable<HashTable> modules_; // Initialized when init() is called.
};

template<typename Tracer>
inline void ModuleRegistry::trace(Tracer&& tracer) {
    tracer(modules_);
}

} // namespace tiro::vm

#endif // TIRO_VM_MODULE_REGISTRY_HPP
