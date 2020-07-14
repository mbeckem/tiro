#ifndef TIRO_VM_OBJECTS_TYPE_DESC_HPP
#define TIRO_VM_OBJECTS_TYPE_DESC_HPP

#include "common/defs.hpp"
#include "common/span.hpp"
#include "vm/objects/fwd.hpp"

namespace tiro::vm {

struct MethodDesc {
    enum Flags {
        // TODO: The variadic flag is ignored by the runtime at the moment, even though
        // it exists here in the metadata. (variadic functions are the default)
        Variadic = 1 << 0,

        // Static methods dont receive an instance parameter.
        Static,
    };

    /// Method name.
    std::string_view name;

    /// Number of required arguments (includes the 'this' argument).
    /// For instance methods, this must always be greater than zero.
    u32 params;

    /// Native function pointer that implements the method.
    NativeFunctionPtr func;

    /// Bitwise combination of `Flags` values.
    int flags = 0;

    constexpr MethodDesc(
        std::string_view name_, u32 params_, NativeFunctionPtr func_, int flags_ = 0)
        : name(name_)
        , params(params_)
        , func(func_)
        , flags(flags_) {}
};

/// Static type description for builtin objects. Descriptors of this type
/// serve as blueprints for the construction of runtime `Type` objects.
/// Note that all members of this class must refer to static data.
struct TypeDesc {
    /// Type name
    std::string_view name;

    /// List of methods.
    Span<const MethodDesc> methods;

    constexpr TypeDesc(std::string_view name_, Span<const MethodDesc> methods_)
        : name(name_)
        , methods(methods_) {}
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_TYPE_DESC_HPP
