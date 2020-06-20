#ifndef TIRO_VM_OBJECTS_MODULES_HPP
#define TIRO_VM_OBJECTS_MODULES_HPP

#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

/// Represents a module, which is a collection of exported and private members.
class Module final : public Value {
public:
    static Module
    make(Context& ctx, Handle<String> name, Handle<Tuple> members, Handle<HashTable> exported);

    Module() = default;

    explicit Module(Value v)
        : Value(v) {
        TIRO_DEBUG_ASSERT(v.is<Module>(), "Value is not a module.");
    }

    String name() const;

    // Contains the private members of this module, e.g. functions, internal variables and constants etc.
    Tuple members() const;

    // Contains exported members, indexed by their name (as a symbol). Exports are constant.
    HashTable exported() const;

    // An invocable function that will be called at module load time.
    Value init() const;
    void init(Handle<Value> value) const;

    // Performs a lookup for the exported module member with that name.
    // Returns an empty optional if no such member was found.
    std::optional<Value> find_exported(Handle<Symbol> name);

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    inline Data* access_heap() const;
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_MODULES_HPP
