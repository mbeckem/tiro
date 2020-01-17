#ifndef TIRO_VM_OBJECTS_FWD_HPP
#define TIRO_VM_OBJECTS_FWD_HPP

#include "tiro/core/defs.hpp"

namespace tiro::vm {

class Value;
class Header;

enum class ValueType : u8;

#define TIRO_VM_TYPE(Name) class Name;
#include "tiro/vm/objects/types.inc"

template<typename Type>
struct MapTypeToValueType;

template<ValueType type>
struct MapValueTypeToType;

template<typename T>
class ArrayVisitor;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_FWD_HPP
