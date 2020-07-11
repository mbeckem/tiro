#ifndef TIRO_VM_OBJECTS_MODULE_HPP
#define TIRO_VM_OBJECTS_MODULE_HPP

#include "vm/handles/handle.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

/// Represents a module, which is a collection of exported and private members.
class Module final : public HeapValue {
private:
    enum {
        NameSlot,
        MembersSlot,
        ExportedSlot,
        InitSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    static Module
    make(Context& ctx, Handle<String> name, Handle<Tuple> members, Handle<HashTable> exported);

    explicit Module(Value v)
        : HeapValue(v, DebugCheck<Module>()) {}

    String name();
    Tuple members();

    // Contains exported members, indexed by their name (as a symbol). Exports are constant.
    HashTable exported();

    // Performs a lookup for the exported module member with that name.
    // Returns an empty optional if no such member was found.
    std::optional<Value> find_exported(Handle<Symbol> name);

    // An invocable function that will be called at module load time.
    Value init();
    void init(MaybeHandle<Value> value);

    Layout* layout() const { return access_heap<Layout>(); }
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_MODULE_HPP
