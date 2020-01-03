#ifndef HAMMER_VM_OBJECTS_OBJECT_HPP
#define HAMMER_VM_OBJECTS_OBJECT_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/span.hpp"
#include "hammer/core/type_traits.hpp"
#include "hammer/vm/heap/handles.hpp"
#include "hammer/vm/objects/value.hpp"

#include <new>
#include <string_view>

namespace hammer::vm {

// FIXME this header needs to go

/// Represents an internal value whose only relevant
/// property is its unique identity.
///
/// TODO: Maybe reuse symbols for this once we have them.
class SpecialValue final : public Value {
public:
    static SpecialValue make(Context& ctx, std::string_view name);

    SpecialValue() = default;

    explicit SpecialValue(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<SpecialValue>(), "Value is not a special value.");
    }

    std::string_view name() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    inline Data* access_heap() const;
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_OBJECT_HPP
