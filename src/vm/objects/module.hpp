#ifndef TIRO_VM_OBJECTS_MODULE_HPP
#define TIRO_VM_OBJECTS_MODULE_HPP

#include "vm/handles/handle.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

/// Represents a reference to another module that has not yet been resolved.
/// UnresolvedImport instances are replaced by the actual modules when a module is initialized.
class UnresolvedImport final : public HeapValue {
private:
    enum {
        ModuleNameSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    static UnresolvedImport make(Context& ctx, Handle<String> module_name);

    explicit UnresolvedImport(Value v)
        : HeapValue(v, DebugCheck<UnresolvedImport>()) {}

    String module_name();

    Layout* layout() const { return access_heap<Layout>(); }
};

/// Represents a module, which is a collection of exported and private members.
/// Modules may reference import other modules or individual members of other modules.
/// Module imports are not resolved immediately when they cannot be satisfied. Import resolution
/// is deferred until the module is actually needed, right before its initialization function is invoked.
///
/// Before a module has been initialized, imports are represented by `UnresolvedImport` instances within
/// the `members` tuple. After successful initialization, those instances will be resolved to references
/// of the actually imported objects.
class Module final : public HeapValue {
private:
    enum {
        NameSlot,
        MembersSlot,
        ExportedSlot,
        InitializerSlot,
        SlotCount_,
    };

    struct Payload {
        bool initialized = false;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static Module
    make(Context& ctx, Handle<String> name, Handle<Tuple> members, Handle<HashTable> exported);

    explicit Module(Value v)
        : HeapValue(v, DebugCheck<Module>()) {}

    // Returns the module name. This is the name that can be used to import this module.
    // It must be unique within the virtual machine.
    String name();

    // Members are private to the module and must not be modified (except by the module itself or during
    // the initialization of the module).
    Tuple members();

    // Contains exported members, indexed by their name (as a symbol). Values are indices into the `members` tuple.
    // Exports are constant and must never be changed.
    HashTable exported();

    // Performs a lookup for the exported module member with that name.
    // Returns an empty optional if no such member was found.
    std::optional<Value> find_exported(Symbol name);

    // A function that will be called at module load time. May be null.
    Value initializer();
    void initializer(Value value);

    // True if the module has been initialized.
    bool initialized();
    void initialized(bool value);

    Layout* layout() const { return access_heap<Layout>(); }
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_MODULE_HPP
