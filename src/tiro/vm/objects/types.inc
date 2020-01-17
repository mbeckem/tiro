// This file lists all vm types used in this library.
// The TIRO_VM_TYPE macro is invoked for every type name.

#ifndef TIRO_VM_TYPE
#    define TIRO_VM_TYPE(Name)
#endif

TIRO_VM_TYPE(Null)
TIRO_VM_TYPE(Undefined)
TIRO_VM_TYPE(Boolean)
TIRO_VM_TYPE(Integer)
TIRO_VM_TYPE(Float)
TIRO_VM_TYPE(SmallInteger)
TIRO_VM_TYPE(String)
TIRO_VM_TYPE(StringBuilder)
TIRO_VM_TYPE(Symbol)
TIRO_VM_TYPE(Code)
TIRO_VM_TYPE(FunctionTemplate)
TIRO_VM_TYPE(ClosureContext)
TIRO_VM_TYPE(Function)
TIRO_VM_TYPE(NativeFunction)
TIRO_VM_TYPE(NativeAsyncFunction)
TIRO_VM_TYPE(NativeObject)
TIRO_VM_TYPE(NativePointer)
TIRO_VM_TYPE(DynamicObject)
TIRO_VM_TYPE(Method)
TIRO_VM_TYPE(BoundMethod)
TIRO_VM_TYPE(Module)
TIRO_VM_TYPE(Tuple)
TIRO_VM_TYPE(Array)
TIRO_VM_TYPE(ArrayStorage)
TIRO_VM_TYPE(Buffer)
TIRO_VM_TYPE(HashTable)
TIRO_VM_TYPE(HashTableStorage)
TIRO_VM_TYPE(HashTableIterator)
TIRO_VM_TYPE(Coroutine)
TIRO_VM_TYPE(CoroutineStack)

#undef TIRO_VM_TYPE
