#ifndef TIRO_VM_HASH_HPP
#define TIRO_VM_HASH_HPP

#include "common/defs.hpp"
#include "common/span.hpp"

namespace tiro::vm {

size_t byte_hash(Span<const byte> data);
size_t integer_hash(u64 data);
size_t float_hash(f64 data);

} // namespace tiro::vm

#endif // TIRO_VM_HASH_HPP
