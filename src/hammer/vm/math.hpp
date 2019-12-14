#ifndef HAMMER_VM_MATH_HPP
#define HAMMER_VM_MATH_HPP

#include "hammer/core/defs.hpp"
#include "hammer/vm/fwd.hpp"
#include "hammer/vm/heap/handles.hpp"
#include "hammer/vm/objects/value.hpp"

#include <optional>

namespace hammer::vm {

// Extracts, but does not convert an integer from v (supports Integer and SmallInteger).
std::optional<i64> try_extract_integer(Value v);
i64 extract_integer(Value v);

// Converts v into an integer (supports all numeric types).
std::optional<i64> try_convert_integer(Value v);
i64 convert_integer(Value v);

// Converts v into a float (supports all numeric types).
std::optional<f64> try_convert_float(Value v);
f64 convert_float(Value v);

// Implements the binary arithmetic operations. Throws on invalid argument.
Value add(Context& ctx, Handle<Value> a, Handle<Value> b);
Value sub(Context& ctx, Handle<Value> a, Handle<Value> b);
Value mul(Context& ctx, Handle<Value> a, Handle<Value> b);
Value div(Context& ctx, Handle<Value> a, Handle<Value> b);
Value mod(Context& ctx, Handle<Value> a, Handle<Value> b);
Value pow(Context& ctx, Handle<Value> a, Handle<Value> b);

// Implements the unary arithmetic operations. Throws on invalid argument.
Value unary_plus(Context& ctx, Handle<Value> v);
Value unary_minus(Context& ctx, Handle<Value> v);

// Implements comparison between two numbers.
// Returns
//  - < 0 iff a < b
//  - = 0 iff a == b
//  - > 0 iff a > b
int compare_numbers(Handle<Value> a, Handle<Value> b);

} // namespace hammer::vm

#endif // HAMMER_VM_MATH_HPP
