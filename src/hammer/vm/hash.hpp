#ifndef HAMMER_VM_HASH_HPP
#define HAMMER_VM_HASH_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/span.hpp"

namespace hammer::vm {

size_t byte_hash(Span<const byte> data);
size_t integer_hash(u64 value);
size_t float_hash(f64 value);

} // namespace hammer::vm

#endif // HAMMER_VM_HASH_HPP
