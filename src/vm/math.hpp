#ifndef TIRO_VM_MATH_HPP
#define TIRO_VM_MATH_HPP

#include "common/defs.hpp"
#include "vm/fwd.hpp"
#include "vm/handles/handle.hpp"
#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

// Implements the binary arithmetic operations. Throws on invalid argument.
Number add(Context& ctx, Handle<Value> a, Handle<Value> b);
Number sub(Context& ctx, Handle<Value> a, Handle<Value> b);
Number mul(Context& ctx, Handle<Value> a, Handle<Value> b);
Number div(Context& ctx, Handle<Value> a, Handle<Value> b);
Number mod(Context& ctx, Handle<Value> a, Handle<Value> b);
Number pow(Context& ctx, Handle<Value> a, Handle<Value> b);

// Implements the unary arithmetic operations. Throws on invalid argument.
Number unary_plus(Context& ctx, Handle<Value> v);
Number unary_minus(Context& ctx, Handle<Value> v);

Integer bitwise_not(Context& ctx, Handle<Value> v);

// Implements comparison between two numbers.
// Returns
//  - < 0 iff a < b
//  - = 0 iff a == b
//  - > 0 iff a > b
int compare_numbers(Value a, Value b);

} // namespace tiro::vm

#endif // TIRO_VM_MATH_HPP
