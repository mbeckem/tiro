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
class ArrayIterator;
class ArrayStorage;
class Boolean;
class BoundMethod;
class Buffer;
class Code;
class Coroutine;
class CoroutineStack;
class CoroutineToken;
class Environment;
class Exception;
class Float;
class Function;
class FunctionTemplate;
class HashTable;
class HashTableIterator;
class HashTableKeyIterator;
class HashTableKeyView;
class HashTableStorage;
class HashTableValueIterator;
class HashTableValueView;
class Integer;
class InternalType;
class Method;
class Module;
class NativeFunction;
class NativeObject;
class NativePointer;
class Null;
class Record;
class RecordTemplate;
class Result;
class Set;
class SetIterator;
class SmallInteger;
class String;
class StringBuilder;
class StringIterator;
class StringSlice;
class Symbol;
class Tuple;
class TupleIterator;
class Type;
class Undefined;
class UnresolvedImport;
// [[[end]]]

class HashTableEntry;

template<typename T>
class Nullable;

template<typename T>
class Fallible;

class NativeFunctionFrame;
class NativeAsyncFunctionFrame;

using NativeFunctionPtr = void (*)(NativeFunctionFrame&);
using NativeAsyncFunctionPtr = void (*)(NativeAsyncFunctionFrame);

struct CoroutineFrame;
struct UserFrame;
struct SyncFrame;
struct AsyncFrame;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_FWD_HPP
