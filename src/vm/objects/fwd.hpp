#ifndef TIRO_VM_OBJECTS_FWD_HPP
#define TIRO_VM_OBJECTS_FWD_HPP

#include "common/defs.hpp"

namespace tiro::vm {

class Value;
class HeapValue;

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
class InternalType;
class Method;
class Module;
class NativeFunction;
class NativeObject;
class NativePointer;
class Null;
class Result;
class SmallInteger;
class String;
class StringBuilder;
class StringSlice;
class Symbol;
class Tuple;
class Type;
class Undefined;
// [[[end]]]

template<typename T>
class Nullable;

template<typename LayoutType>
struct LayoutTraits;

class NativeFunctionFrame;
class NativeAsyncFunctionFrame;

using NativeFunctionPtr = void (*)(NativeFunctionFrame&);
using NativeAsyncFunctionPtr = void (*)(NativeAsyncFunctionFrame);

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_FWD_HPP
