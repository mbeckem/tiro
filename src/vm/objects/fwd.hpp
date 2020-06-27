#ifndef TIRO_VM_OBJECTS_FWD_HPP
#define TIRO_VM_OBJECTS_FWD_HPP

#include "common/defs.hpp"

namespace tiro::vm {

class Value;
class HeapValue;
class Header;

enum class ValueType : u8;

/* [[[cog
    from cog import outl
    from codegen.objects import VM_OBJECTS
    for object in VM_OBJECTS:
        outl(f"class {object.type_name};")
]]] */
class Array;
class ArrayStorage;
class Boolean;
class BoundMethod;
class Buffer;
class Code;
class Coroutine;
class CoroutineStack;
class DynamicObject;
class Environment;
class Float;
class Function;
class FunctionTemplate;
class HashTable;
class HashTableIterator;
class HashTableStorage;
class Integer;
class Method;
class Module;
class NativeAsyncFunction;
class NativeFunction;
class NativeObject;
class NativePointer;
class Null;
class SmallInteger;
class String;
class StringBuilder;
class Symbol;
class Tuple;
class Type;
class Undefined;
// [[[end]]]

template<typename T>
class ArrayVisitor;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_FWD_HPP
