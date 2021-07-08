#ifndef TIRO_VM_BUILTINS_UTILS_HPP
#define TIRO_VM_BUILTINS_UTILS_HPP

#include "vm/fwd.hpp"
#include "vm/handles/fwd.hpp"
#include "vm/objects/fwd.hpp"

namespace tiro::vm {

Fallible<> check_arity(Context& ctx, std::string_view function_name, size_t expected_argc,
    const NativeFunctionFrame& frame);

Fallible<> check_arity(
    std::string_view function_name, size_t expected_argc, const NativeAsyncFunctionFrame& frame);

template<typename T>
Fallible<>
check_type(std::string_view function_name, std::string_view var_name, Handle<Value> value) {}

} // namespace tiro::vm

#endif // TIRO_VM_BUILTINS_UTILS_HPP
