#ifndef TIRO_VM_OBJECT_SUPPORT_TYPE_DESC_HPP
#define TIRO_VM_OBJECT_SUPPORT_TYPE_DESC_HPP

#include "common/adt/span.hpp"
#include "common/defs.hpp"
#include "vm/objects/fwd.hpp"

namespace tiro::vm {

using SyncFunctionPtr = void (*)(SyncFrameContext& frame);
using AsyncFunctionPtr = void (*)(AsyncFrameContext& frame);
using ResumableFunctionPtr = void (*)(ResumableFrameContext& frame);

enum class FunctionPtrType { Sync, Async, Resumable };

struct FunctionPtr {
    FunctionPtrType type;
    union {
        SyncFunctionPtr sync;
        AsyncFunctionPtr async;
        struct {
            ResumableFunctionPtr func;
            u32 locals;
        } resumable;
    };

    constexpr FunctionPtr(SyncFunctionPtr ptr)
        : type(FunctionPtrType::Sync)
        , sync(ptr) {}

    constexpr FunctionPtr(AsyncFunctionPtr ptr)
        : type(FunctionPtrType::Async)
        , async(ptr) {}

    constexpr FunctionPtr(ResumableFunctionPtr ptr)
        : FunctionPtr(ptr, 0) {}

    constexpr FunctionPtr(ResumableFunctionPtr ptr, u32 locals)
        : type(FunctionPtrType::Resumable)
        , resumable({ptr, locals}) {}
};

struct FunctionDesc {
    enum Flags {
        // TODO: The variadic flag is ignored by the runtime at the moment, even though
        // it exists here in the metadata. (variadic functions are the default)
        Variadic = 1 << 0,

        /// Methods receive an instance parameter.
        /// Their argument count must be at least 1.
        InstanceMethod = 1 << 1,
    };

    /// Function name.
    std::string_view name;

    /// Number of required arguments (includes the 'this' argument).
    /// For instance methods, this must always be greater than zero.
    u32 params;

    /// Native function pointer argument that implements the function.
    FunctionPtr func;

    /// Bitwise combination of `Flags` values.
    int flags = 0;

    static constexpr FunctionDesc
    method(std::string_view name, u32 params, const FunctionPtr& func, int flags = 0) {
        return {name, params, func, flags | InstanceMethod};
    }

    static constexpr FunctionDesc
    static_method(std::string_view name, u32 params, const FunctionPtr& func, int flags = 0) {
        TIRO_DEBUG_ASSERT((flags & InstanceMethod) == 0,
            "Must not set the instance method flag in static methods");
        return {name, params, func, flags};
    }

    static constexpr FunctionDesc
    plain(std::string_view name, u32 params, const FunctionPtr& func, int flags = 0) {
        TIRO_DEBUG_ASSERT((flags & InstanceMethod) == 0,
            "Must not set the instance method flag in plain function");
        return {name, params, func, flags};
    }

private:
    constexpr FunctionDesc(
        std::string_view name_, u32 params_, const FunctionPtr& func_, int flags_ = 0)
        : name(name_)
        , params(params_)
        , func(func_)
        , flags(flags_) {
#if TIRO_DEBUG
        if (flags & InstanceMethod)
            TIRO_DEBUG_ASSERT(params > 0, "Must have at least one parameter");
#endif
    }
};

/// Static type description for builtin objects. Descriptors of this type
/// serve as blueprints for the construction of runtime `Type` objects.
/// Note that all members of this class must refer to static data.
struct TypeDesc {
    /// Type name
    std::string_view name;

    /// List of methods.
    Span<const FunctionDesc> methods;

    constexpr TypeDesc(std::string_view name_, Span<const FunctionDesc> methods_)
        : name(name_)
        , methods(methods_) {}
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECT_SUPPORT_TYPE_DESC_HPP
