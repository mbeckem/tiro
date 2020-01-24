#ifndef TIRO_VM_HASH_HPP
#define TIRO_VM_HASH_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/span.hpp"

namespace tiro::vm {

size_t byte_hash(Span<const byte> data);
size_t integer_hash(u64 value);
size_t float_hash(f64 value);

} // namespace tiro::vm

#endif // TIRO_VM_HASH_HPP