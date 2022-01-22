#ifndef TIRO_VM_ROOT_SET_HPP
#define TIRO_VM_ROOT_SET_HPP

#include "common/defs.hpp"
#include "vm/fwd.hpp"
#include "vm/handles/external.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/scope.hpp"
#include "vm/interpreter.hpp"
#include "vm/modules/registry.hpp"
#include "vm/objects/nullable.hpp"
#include "vm/objects/value.hpp"
#include "vm/type_system.hpp"

namespace tiro::vm {

/// Contains the gc roots.
/// Tracing starts here and continues until all reachable values have been visited.
class RootSet final {
public:
    RootSet();
    ~RootSet();

    RootSet(const RootSet&) = delete;
    RootSet& operator=(const RootSet&) = delete;

    /// Initializes this root set with a reference to its owning context.
    /// There is only one context for every set of roots.
    /// Must be called before any other method of this class.
    void init(Context& ctx);

    Handle<Boolean> get_true();
    Handle<Boolean> get_false();
    Handle<Undefined> get_undefined();
    Handle<HashTable> get_interned_strings();
    Handle<Set> get_coroutines();

    MutHandle<Nullable<Coroutine>> get_first_ready();
    MutHandle<Nullable<Coroutine>> get_last_ready();

    RootedStack& get_stack() { return stack_; }
    ExternalStorage& get_externals() { return externals_; }
    Interpreter& get_interpreter() { return interpreter_; }
    TypeSystem& get_types() { return types_; }
    ModuleRegistry& get_modules() { return modules_; }

    template<typename Tracer>
    inline void trace(Tracer&& tracer);

private:
    Nullable<Boolean> true_;
    Nullable<Boolean> false_;
    Nullable<Undefined> undefined_;
    Nullable<Coroutine> first_ready_, last_ready_; // Linked list of runnable coroutines
    Nullable<HashTable> interned_strings_;         // TODO this should eventually be a weak map

    // Created and not yet completed coroutines.
    Nullable<Set> coroutines_;

    // Stack of values used for Scope/Local instances.
    RootedStack stack_;

    // Set of potentially long lived handles, used e.g. in the public API.
    ExternalStorage externals_;

    // The current interpreter. NOTE: there should be more than one.
    Interpreter interpreter_;

    // Types registered with the vm.
    TypeSystem types_;

    // Modules registered with the vm.
    ModuleRegistry modules_;
};

} // namespace tiro::vm

#endif // TIRO_VM_ROOT_SET_HPP
