#ifndef HAMMER_VM_OBJECTS_FWD_HPP
#define HAMMER_VM_OBJECTS_FWD_HPP

#include "hammer/core/defs.hpp"

namespace hammer::vm {

class Value;
class Header;
class WriteBarrier;

enum class ValueType : u8;

#define HAMMER_VM_TYPE(Name) class Name;
#include "hammer/vm/objects/types.inc"

template<typename Type>
struct MapTypeToValueType;

template<ValueType type>
struct MapValueTypeToType;

template<typename T>
class ArrayVisitor;

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_FWD_HPP
