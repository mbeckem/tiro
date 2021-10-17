#ifndef TIRO_VM_MATH_HPP
#define TIRO_VM_MATH_HPP

#include "common/defs.hpp"
#include "vm/fwd.hpp"

namespace tiro::vm {

// Implements the binary arithmetic operations.
Fallible<Number> add(Context& ctx, Handle<Value> a, Handle<Value> b);
Fallible<Number> sub(Context& ctx, Handle<Value> a, Handle<Value> b);
Fallible<Number> mul(Context& ctx, Handle<Value> a, Handle<Value> b);
Fallible<Number> div(Context& ctx, Handle<Value> a, Handle<Value> b);
Fallible<Number> mod(Context& ctx, Handle<Value> a, Handle<Value> b);
Fallible<Number> pow(Context& ctx, Handle<Value> a, Handle<Value> b);

// Implements the unary arithmetic operations.
Fallible<Number> unary_plus(Context& ctx, Handle<Value> v);
Fallible<Number> unary_minus(Context& ctx, Handle<Value> v);

Fallible<Integer> bitwise_not(Context& ctx, Handle<Value> v);

// Implements comparison between two objects.
// Returns
//  - < 0 iff a < b
//  - = 0 iff a == b
//  - > 0 iff a > b
Fallible<int> compare(Context& ctx, Handle<Value> a, Handle<Value> b);

} // namespace tiro::vm

#endif // TIRO_VM_MATH_HPP
