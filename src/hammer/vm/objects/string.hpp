#ifndef HAMMER_VM_OBJECTS_STRING_HPP
#define HAMMER_VM_OBJECTS_STRING_HPP

#include "hammer/vm/objects/value.hpp"

namespace hammer::vm {

/**
 * Represents a string.
 * 
 * TODO: Unicode stuff.
 */
class String final : public Value {
public:
    static String make(Context& ctx, std::string_view str);

    String() = default;

    explicit String(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<String>(), "Value is not a string.");
    }

    std::string_view view() const noexcept { return {data(), size()}; }

    const char* data() const noexcept;
    size_t size() const noexcept;

    size_t hash() const noexcept;

    bool equal(String other) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_STRING_HPP
